/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of C++/Tcl common functions.
 */

#include <set>
#include <map>
#include <vector>
#include <stack>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <csetjmp>
#include <csignal>
#include <typeinfo>
#include <exception>
#include <stdexcept>
#include <iostream>

#include <tcl.h>
#include <itcl.h>

#include <SDL.h>

#include "src/core/aobject.hxx"
#include "bridge.hxx"
#include "implementation.hxx"
#include "src/exit_conditions.hxx"
#include "src/globals.hxx"

using namespace std;
using namespace tcl_glue_implementation;

//Things from bridge.hxx
THREAD_LOCAL JmpBuf* currentTclErrorHandler_ptr;
THREAD_LOCAL stack<JmpBuf>* tclErrorHandlerStack_ptr;
THREAD_LOCAL const char* scriptingErrorMessage;

void prepareTclBridge() {
  mutex = SDL_CreateMutex();
}

namespace tcl_glue_implementation {
THREAD_LOCAL char staticError[1024];

/* Some Tcl modules are, oddly, not thread safe (eg, TLS).
 * We need to make sure to only load one interpreter at a time.
 */
SDL_mutex* mutex;

//Make setting a string result more sensible
#define Tcl_SetResult(interp, str, ignore) Tcl_SetObjResult(interp, Tcl_NewStringObj(str, strlen(str)+1))

}

void scriptError(const char* err) {
  scriptingErrorMessage=err;
  longjmp(currentTclErrorHandler, 1);
}
namespace tcl_glue_implementation {

  /* Map type_infos to our export information. This is
   * implemented as a map primarily to allow for discovery
   * of the dynamic type of Abendstern objects at export-
   * time; for simplicity and consistency, it also holds
   * static foreign types.
   */
  THREAD_LOCAL map<const type_info*, TypeExport*>* typeExports_ptr;
  THREAD_LOCAL Interpreters* interpreters_ptr;

  Tcl_Interp* newInterpreterImpl(bool safe, Tcl_Interp* master) {
    SDL_mutexP(mutex);
    //Generate new infos and magic cookie
    InterpInfo* info=new InterpInfo;
    info->enforceAllocationLimit=safe;
    for (unsigned  i=0; i<sizeof(info->magicCookie)-1; ++i) {
      info->magicCookie[i] = 'A' + (rand() % ('Z'-'A'));
    }
    info->magicCookie[sizeof(info->magicCookie)-1]=0;
    stringstream ss(ios::out);
    ss << "slave" << rand();
    info->slaveName = ss.str();
    Tcl_Interp* interp;
    if (!master) {
      interp=Tcl_CreateInterp();
      info->interpreter=interp;
    } else {
      interp=Tcl_CreateSlave(master, info->slaveName.c_str(), false);
      info->interpreter=interp;
    }
    interpreters[interp]=info;
    if (Tcl_Init(interp) == TCL_ERROR) {
      cerr << "FATAL: Unable to initialize Tcl: " << Tcl_GetStringResult(interp) << endl;
      exit(EXIT_PLATFORM_ERROR);
    }
    if (Itcl_Init(interp) == TCL_ERROR) {
      cerr << "FATAL: Unable to initialize [incr Tcl]: " << Tcl_GetStringResult(interp) << endl;
      exit(EXIT_PLATFORM_ERROR);
    }
    if (safe) Tcl_MakeSafe(info->interpreter);
    SDL_mutexV(mutex);
    return interp;
  }

  void newInterpreterImplPost(Tcl_Interp* interp) {
    InterpInfo* info=interpreters[interp];
    Tcl_SetVar(interp, "ABENDSTERN-MAGIC-COOKIE-1", info->magicCookie, TCL_GLOBAL_ONLY);
    if (TCL_ERROR == Tcl_EvalFile(interp, "tcl/bridge.tcl")) {
      cerr << "FATAL: Unable to load tcl/bridge.tcl: " << Tcl_GetStringResult(interp) << endl;
      Tcl_Obj* options=Tcl_GetReturnOptions(interp, TCL_ERROR);
      Tcl_Obj* key=Tcl_NewStringObj("-errorinfo", -1);
      Tcl_Obj* stackTrace;
      Tcl_IncrRefCount(key);
      Tcl_DictObjGet(NULL, options, key, &stackTrace);
      Tcl_DecrRefCount(key);
      cerr << "Stack trace:\n" << Tcl_GetStringFromObj(stackTrace, NULL) << endl;
      exit(EXIT_SCRIPTING_BUG);
    }
  }

  //definition.tcl needs to define this as {c++ delete}
  void cppDelete(const string& name, const string& magicCookie) {
    InterpInfo* info=interpreters[invokingInterpreter];
    Export* ex;
    map<string,Export*>::iterator it=info->exportsByName.find(name);
    if (it==info->exportsByName.end()) {
      scriptError("Attempt to delete non-existent object");
    }
    if (magicCookie != info->magicCookie) {
      scriptError("Invalid magic cookie passed to {c++ delete}");
    }
    ex = (*it).second;
    //Only check for C++-side deletion if is an AObject
    if (ex->type->isAObject) {
      AObject* ao=(AObject*)ex->ptr;
      if (ao->ownStat == AObject::Tcl
      &&  ao->owner.interpreter == invokingInterpreter) {
        //Unconditionally OK to remove
        //We MUST remove the export mappings first, though,
        //to avert infinite corecursion
        info->exportsByName.erase(it);
        info->exports.erase(info->exports.find(ex->ptr));
        delete ex;
        //C++ now owns it until it is deleted
        ao->ownStat=AObject::Cpp;
        delete ao;
      } else {
        //Only the export to this Tcl is being removed.
        //However, we MUST make sure that this interpreter
        //is not extending the object (otherwise, it would
        //invalid all references thereto)
        if (ao->tclExtended == invokingInterpreter) {
          scriptError("Attempt to delete Tcl-extended object by extending interpreter when not owned thereby");
        }
        //OK
        info->exportsByName.erase(it);
        info->exports.erase(info->exports.find(ex->ptr));
        delete ex;
      }
    } else {
      //Foreign, just kill the reference
      info->exportsByName.erase(it);
      info->exports.erase(info->exports.find(ex->ptr));
      delete ex;
    }
  }

  //For debugging (callable by Tcl)
  void debugTclExports() {
    InterpInfo* info = interpreters[invokingInterpreter];
    cout << "--- BEGIN: ptr ==> name ---" << endl;
    for (map<void*,Export*>::const_iterator it = info->exports.begin();
         it != info->exports.end(); ++it)
      cout << it->first << '\t' << it->second->tclrep
           << "\t(" << it->second->ptr << ")" << endl;
    cout << "--- END: ptr ==> name ---" << endl;
    cout << "--- BEGIN: name ==> ptr ---" << endl;
    for (map<string,Export*>::const_iterator it = info->exportsByName.begin();
         it != info->exportsByName.end(); ++it)
      cout << it->first << '\t' << it->second->ptr
           << "\t(" << it->second->tclrep << ")" << endl;
    cout << "--- END: name ==> ptr ---" << endl;
  }
}

void delInterpreter(Tcl_Interp* interp) {
  InterpInfo* info=interpreters[interp];
  //Delete anly slaves
  vector<Tcl_Interp*> slaves;
  for (map<Tcl_Interp*,InterpInfo*>::iterator it=interpreters.begin();
       it != interpreters.end(); ++it)
    if (Tcl_GetMaster((*it).first) == interp)
      slaves.push_back((*it).first);
  for (unsigned i=0; i<slaves.size(); ++i)
    delInterpreter(slaves[i]);

  //Delete owned objects (all of which will be AObjects)
  map<void*,Export*> exports(info->exports); //Copy, since we'll be modifying the original
  for (map<void*,Export*>::iterator it=exports.begin();
       it != exports.end(); ++it)
  {
    Export* ex=(*it).second;
    if (ex->type->isAObject) {
      AObject* ao=(AObject*)ex->ptr;
      if (ao->ownStat == AObject::Tcl && ao->owner.interpreter == interp)
        delete ao; //This will cause the corresponding Export* to be deleted
      //Handle case of resident Tcl object
      else if (ao->tclExtended)
        throw runtime_error("Unexpected attempt to free interpreter with C++-owned Tcl value extending C++ type");
      //Any other Exports are owned by the Interpreter, but the C++ object is not,
      //so just free the export
      else delete ex;
    }
  }

  //Finally, free the interpreter
  Tcl_DeleteInterp(interp);
  interpreters.erase(interpreters.find(interp));
}

void interpSafeSource(const char* path) {
  if (TCL_ERROR == Tcl_EvalFile(invokingInterpreter, path)) {
    //Hide information about the error
    throw runtime_error("Unable to safe_source file");
  }
}

bool validateSafeSourcePath(const char* path) {
  if (!path) return false;
  if (strlen(path) < 8 || strlen(path) > 64) return false;

  unsigned dotCnt=0;
  bool lastWasDot=false;
  if (path[0] != 't' || path[1] != 'c' || path[2] != 'l' || path[3] != '/')
    return false;
  for (char c=*path; c; c=*++path) {
    if (c < 'a' && c > 'z' && c != '.' && c != '/' && c != '_') return false;
    if (c == '.') {
      ++dotCnt;
      lastWasDot=true;
      if (*(path-1) == '/')
        return false;
      if (*(path+1) == '/')
        return false;
    } else {
      lastWasDot=false;
    }
  }

  return dotCnt <= 1 && !lastWasDot;
}

THREAD_LOCAL Tcl_Interp* invokingInterpreter;

AObject::~AObject() {
  if (!tclKnown) return;
  if (interpreters.destroyed) return;
  if (ownStat != Cpp && ownStat != Container) {
    cerr << "FATAL: Attempt to delete AObject not owned by C++!" << endl;
    exit(EXIT_PROGRAM_BUG);
  }

  //Delete any existing exports to it
  //To prevent issues with Tcl-extended objects, pretend we aren't
  //extended by Tcl; since c++ owns us, {c++ delete} will not try
  //to delete us in C++ form, preventing infinite corecursion.
  tclExtended=NULL;
  for (map<Tcl_Interp*,InterpInfo*>::iterator it=interpreters.begin();
       it != interpreters.end(); ++it)
  {
    InterpInfo* info=(*it).second;
    map<void*,Export*>::iterator exit = info->exports.find((void*)this);
    if (exit != info->exports.end()) {
      Export* ex=(*exit).second;
      //Call ::itcl::delete object <name>; {c++ delete} will
      //remove the Export* for us
      Tcl_Obj* cmd=Tcl_NewStringObj("::itcl::delete", -1),
             * sel=Tcl_NewStringObj("object", -1),
             * nam=Tcl_NewStringObj(ex->tclrep.c_str(), ex->tclrep.size());
      Tcl_Obj* objv[3] = {cmd,sel,nam};
      for (unsigned i=0; i<lenof(objv); ++i) Tcl_IncrRefCount(objv[i]);
      //On error, only print a warning
      if (TCL_ERROR==Tcl_EvalObjv(info->interpreter, 3, objv, TCL_EVAL_GLOBAL)) {
        cerr << "ERROR: While deleting C++ reference from Tcl: " << Tcl_GetStringResult(info->interpreter) << endl;
      }
      for (unsigned i=0; i<lenof(objv); ++i) Tcl_DecrRefCount(objv[i]);
    }
  }
}

void foreignLoseControl(void* ptr) {
  for (map<Tcl_Interp*,InterpInfo*>::iterator it=interpreters.begin();
       it != interpreters.end(); ++it)
  {
    InterpInfo* info=(*it).second;
    map<void*,Export*>::iterator exit=info->exports.find(ptr);
    if (exit != info->exports.end()) {
      //Deregister (calling Tcl will takere of deregistering and deleting)
      Export* ex=(*exit).second;
      Tcl_Obj* cmd=Tcl_NewStringObj("::itcl::delete", -1),
             * sel=Tcl_NewStringObj("object", -1),
             * nam=Tcl_NewStringObj(ex->tclrep.c_str(), ex->tclrep.size());
      Tcl_Obj* objv[3] = {cmd,sel,nam};
      //On error, only print a warning
      if (TCL_ERROR==Tcl_EvalObjv(info->interpreter, 3, objv, TCL_EVAL_GLOBAL)) {
        cerr << "ERROR: While deleting C++ reference from Tcl: " << Tcl_GetStringResult(info->interpreter) << endl;
      }
    }
  }
}

void initBridgeTLS() {
  currentTclErrorHandler_ptr = new JmpBuf;
  tclErrorHandlerStack_ptr = new stack<JmpBuf>;
  typeExports_ptr = new map<const type_info*,TypeExport*>;
  interpreters_ptr = new Interpreters;
}

#ifdef DEBUG
  void tcl_glue_implementation::commitSuicideInsteadOfLongJump(jmp_buf,int) {
    cout << scriptingErrorMessage << endl;
    ++*(int*)NULL;
  }
  //#define longjmp commitSuicideInsteadOfLongJump
#endif
