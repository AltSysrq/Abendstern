
  /**
   * @file
   * @author C++-Tcl Bridge Code Generator
   * @brief Autogenerated bindings; <b>not intended for human consumption</b>
   *
   * AUTOGENERATED BY generate.tcl. DO NOT EDIT DIRECTLY.
   * @see src/tcl_iface/readme.txt
   */

  #include <map>
  #include <set>
  #include <vector>
  #include <string>
  #include <cstring>
  #include <cstdio>
  #include <cstdlib>
  #include <iostream>

  #include <GL/gl.h>
  #include <SDL.h>
  #include <tcl.h>
  #include <itcl.h>
  #include <libconfig.h++>

  #include "src/tcl_iface/bridge.hxx"
  #include "src/tcl_iface/implementation.hxx"
  #include "src/tcl_iface/dynfun.hxx"
  #include "src/exit_conditions.hxx"
  #include "src/globals.hxx"

  #pragma GCC diagnostic ignored "-Wunused-label"
  #pragma GCC diagnostic ignored "-Waddress"
  using namespace std;
  using namespace tcl_glue_implementation;
  using namespace libconfig;

  //Commands get their zeroth argument as their own name;
  //code generation is simpler if we drop this
  #define SHIFT ++objv, --objc

#include "src/background/old_style_explosion.hxx"
#include "src/background/explosion.hxx"
#include "src/background/star_field.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/game_field.hxx"
#include "src/camera/hud.hxx"
#include "src/net/async_ack_geraet.hxx"

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
 int
     trampoline1 (
     ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv[]) throw() {
       SHIFT;
       if (objc != 2) {
         Tcl_SetResult(interp, "Incorrect number of arguments passed to internal function", TCL_VOLATILE);
         return TCL_ERROR;
       }
       invokingInterpreter=interp;
       
string arg0; bool arg0Init=false;
string arg1; bool arg1Init=false;
PUSH_TCL_ERROR_HANDLER(errorOccurred); if (errorOccurred) goto error;

{
      static Tcl_DString dstr;
      static bool hasDstr=false;
      if (!hasDstr) {
        Tcl_DStringInit(&dstr);
        hasDstr=true;
      } else {
        Tcl_DStringSetLength(&dstr, 0);
      }
      int length;
      Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(objv[0], &length);
      arg0 = Tcl_UniCharToUtfDString(tuc, length, &dstr);
    };
arg0Init=true;
{
      static Tcl_DString dstr;
      static bool hasDstr=false;
      if (!hasDstr) {
        Tcl_DStringInit(&dstr);
        hasDstr=true;
      } else {
        Tcl_DStringSetLength(&dstr, 0);
      }
      int length;
      Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(objv[1], &length);
      arg1 = Tcl_UniCharToUtfDString(tuc, length, &dstr);
    };
arg1Init=true;
try {
      
     
     
     cppDelete(arg0, arg1);

    } catch (exception& ex) {
      sprintf(staticError, "%s: %s", typeid(ex).name(), ex.what());
      scriptError(staticError);
    }
POP_TCL_ERROR_HANDLER;
      return TCL_OK;
error:
      POP_TCL_ERROR_HANDLER;
      double_error:
      #undef scriptError
      #define scriptError(msg) { \
        cerr << "Double-error; old message: " << scriptingErrorMessage << \
        ", new message: " << msg << endl; \
        scriptingErrorMessage = msg; goto double_error; \
      }
      
if (arg0Init) {arg0Init=false; }
if (arg1Init) {arg1Init=false; }
#undef scriptError
Tcl_SetResult(interp, scriptingErrorMessage, NULL); return TCL_ERROR; }
#undef scriptError

class TclOldStyleExplosion : public OldStyleExplosion {
      public:
TclOldStyleExplosion(Explosion* arg2) : OldStyleExplosion(arg2) {
        tclExtended=invokingInterpreter;
        tclKnown=true;
      }
static OldStyleExplosion* constructordefault
    (const string& name, const string& magicCookie,  Explosion* arg2) {
      InterpInfo* info=interpreters[invokingInterpreter];
      //Validate magic cookie
      if (magicCookie != info->magicCookie) {
        scriptError("Call to c++ new with invalid magic cookie!");
      }

      //Refuse if we enforce allocation and the interpreter knows too much
      if (info->enforceAllocationLimit && info->exports.size() > MAX_EXPORTS) {
        scriptError("Interpreter allocation limit exceeded");
      }

      //Refuse duplicate allocation
      if (info->exportsByName[name]) {
        scriptError("Double allocation");
      }
      OldStyleExplosion* ret;
      ret=new OldStyleExplosion(arg2);
      if (ret) {
        ret->tclKnown = true;
        //The trampoline will take care of assigning ownership to Tcl, we
        //just need to create the export so it has the correct name
        Export* ex=new Export;
        info->exports[(void*)ret]=ex;
        info->exportsByName[name]=ex;
        ex->ptr = (void*)ret;
        ex->type = typeExports[&typeid(OldStyleExplosion)];
        ex->interp = invokingInterpreter;
        ex->tclrep=name;
      }

      return ret;
    }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
static int
     trampoline416 (
     ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv[]) throw() {
       SHIFT;
       if (objc != 3) {
         Tcl_SetResult(interp, "Incorrect number of arguments passed to internal function", TCL_VOLATILE);
         return TCL_ERROR;
       }
       invokingInterpreter=interp;
       
string arg0; bool arg0Init=false;
string arg1; bool arg1Init=false;
Explosion* arg2; bool arg2Init=false;
OldStyleExplosion* ret; Tcl_Obj* retTcl=NULL;
PUSH_TCL_ERROR_HANDLER(errorOccurred); if (errorOccurred) goto error;

{
      static Tcl_DString dstr;
      static bool hasDstr=false;
      if (!hasDstr) {
        Tcl_DStringInit(&dstr);
        hasDstr=true;
      } else {
        Tcl_DStringSetLength(&dstr, 0);
      }
      int length;
      Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(objv[0], &length);
      arg0 = Tcl_UniCharToUtfDString(tuc, length, &dstr);
    };
arg0Init=true;
{
      static Tcl_DString dstr;
      static bool hasDstr=false;
      if (!hasDstr) {
        Tcl_DStringInit(&dstr);
        hasDstr=true;
      } else {
        Tcl_DStringSetLength(&dstr, 0);
      }
      int length;
      Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(objv[1], &length);
      arg1 = Tcl_UniCharToUtfDString(tuc, length, &dstr);
    };
arg1Init=true;
{
      string name(Tcl_GetStringFromObj(objv[2], NULL));
      if (name != "0") {
        //Does it exist?
        InterpInfo* info=interpreters[interp];
        map<string,Export*>::iterator it=info->exportsByName.find(name);
        if (it == info->exportsByName.end()) {
          for (it=info->exportsByName.begin();
               it != info->exportsByName.end(); ++it) {
            cout << (*it).first << endl;
          }
          sprintf(staticError, "Invalid export passed to C++: %s",
                  name.c_str());
          scriptError(staticError);
        }
        Export* ex=(*it).second;
        //OK, is the type correct?
        if (ex->type->theType != typeid(Explosion)
        &&  0==ex->type->superclasses.count(&typeid(Explosion))) {
          //Nope
          sprintf(staticError, "Wrong type passed to C++ function; expected"
                               " Explosion, "
                               "got %s", ex->type->tclClassName.c_str());
          scriptError(staticError);
        }

        //All is well, transfer ownership now
        Explosion* tmp=(Explosion*)ex->ptr;
        
        arg2 = tmp;
    } else arg2=NULL;
};
arg2Init=true;
try {
      ret =
     
     
     constructordefault(arg0, arg1, arg2);

    } catch (exception& ex) {
      sprintf(staticError, "%s: %s", typeid(ex).name(), ex.what());
      scriptError(staticError);
    }
{if (!(ret)) {
      retTcl=Tcl_NewStringObj("0", 1);
    } else {
      InterpInfo* info=interpreters[interp];
      //Is it a valid export already?
      void* ptr=const_cast<void*>((void*)(ret));
      map<void*,Export*>::iterator it=info->exports.find(ptr);
      Export* ex;
      if (it == info->exports.end()) {
        //No, create new one
        (ret)->tclKnown=true;
        ex=new Export;
        ex->ptr=ptr;
        ex->type=typeExports[&typeid(*(ret))];
        ex->interp=interp;
        

        //Create the new Tcl-side object with
        //  new Type {}
        Tcl_Obj* cmd[3] = {
          Tcl_NewStringObj("new", 3),
          Tcl_NewStringObj(ex->type->tclClassName.c_str(),
                           ex->type->tclClassName.size()),
          Tcl_NewObj(),
        };
        for (unsigned i=0; i<lenof(cmd); ++i)
          Tcl_IncrRefCount(cmd[i]);
        int status=Tcl_EvalObjv(interp, lenof(cmd), cmd, TCL_EVAL_GLOBAL);
        for (unsigned i=0; i<lenof(cmd); ++i)
          Tcl_DecrRefCount(cmd[i]);
        if (status == TCL_ERROR) {
          sprintf(staticError, "Error exporting C++ object to Tcl: %s",
                  Tcl_GetStringResult(interp));
          scriptError(staticError);
        }

        //We can now get the name, and then have a fully-usable export
        ex->tclrep=Tcl_GetStringResult(interp);
        //Register
        info->exports[(void*)(ret)]=ex;
        info->exportsByName[ex->tclrep]=ex;
      } else {
        //Yes, use directly
        ex = (*it).second;
      }

      //Ownership
      if ((ret)) switch ((ret)->ownStat) {
            case AObject::Tcl:
              if (interp == (ret)->owner.interpreter) {
                
              }
              //fall through
            case AObject::Cpp:
              (ret)->ownStatBak=(ret)->ownStat;
              (ret)->ownerBak.interpreter = (ret)->owner.interpreter;
              (ret)->ownStat=AObject::Tcl;
              (ret)->owner.interpreter=interp;
              break;
            case AObject::Container:
              cerr <<
"FATAL: Attempt by C++ to give Tcl ownership of automatic value" << endl;
              ::exit(EXIT_PROGRAM_BUG);
          }

      //Done
      retTcl=Tcl_NewStringObj(ex->tclrep.c_str(), ex->tclrep.size());
    }}
Tcl_SetObjResult(interp, retTcl);
POP_TCL_ERROR_HANDLER;
      return TCL_OK;
error:
      POP_TCL_ERROR_HANDLER;
      double_error:
      #undef scriptError
      #define scriptError(msg) { \
        cerr << "Double-error; old message: " << scriptingErrorMessage << \
        ", new message: " << msg << endl; \
        scriptingErrorMessage = msg; goto double_error; \
      }
      
if (arg0Init) {arg0Init=false; }
if (arg1Init) {arg1Init=false; }
if (arg2Init) {arg2Init=false; }
#undef scriptError
Tcl_SetResult(interp, scriptingErrorMessage, NULL); return TCL_ERROR; }
#undef scriptError


#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
static int
     trampoline418 (
     ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv[]) throw() {
       SHIFT;
       if (objc != 2) {
         Tcl_SetResult(interp, "Incorrect number of arguments passed to internal function", TCL_VOLATILE);
         return TCL_ERROR;
       }
       invokingInterpreter=interp;
       OldStyleExplosion* parent=NULL;
float arg0; bool arg0Init=false;
bool ret; Tcl_Obj* retTcl=NULL;
PUSH_TCL_ERROR_HANDLER(errorOccurred); if (errorOccurred) goto error;
{
      string name(Tcl_GetStringFromObj(objv[0], NULL));
      if (name != "0") {
        //Does it exist?
        InterpInfo* info=interpreters[interp];
        map<string,Export*>::iterator it=info->exportsByName.find(name);
        if (it == info->exportsByName.end()) {
          for (it=info->exportsByName.begin();
               it != info->exportsByName.end(); ++it) {
            cout << (*it).first << endl;
          }
          sprintf(staticError, "Invalid export passed to C++: %s",
                  name.c_str());
          scriptError(staticError);
        }
        Export* ex=(*it).second;
        //OK, is the type correct?
        if (ex->type->theType != typeid(OldStyleExplosion)
        &&  0==ex->type->superclasses.count(&typeid(OldStyleExplosion))) {
          //Nope
          sprintf(staticError, "Wrong type passed to C++ function; expected"
                               " OldStyleExplosion, "
                               "got %s", ex->type->tclClassName.c_str());
          scriptError(staticError);
        }

        //All is well, transfer ownership now
        OldStyleExplosion* tmp=(OldStyleExplosion*)ex->ptr;
        
        parent = tmp;
    } else parent=NULL;
}
      if (!parent) { scriptError("NULL this passed into C++"); }
{double tmp;
            int err = Tcl_GetDoubleFromObj(interp, objv[1], &tmp);
            if (err == TCL_ERROR)
              scriptError(Tcl_GetStringResult(interp));
            arg0 = (float)tmp;};
arg0Init=true;
try {
      ret =
     parent->
     
     update(arg0);

    } catch (exception& ex) {
      sprintf(staticError, "%s: %s", typeid(ex).name(), ex.what());
      scriptError(staticError);
    }
{retTcl = Tcl_NewBooleanObj(ret);}
Tcl_SetObjResult(interp, retTcl);
POP_TCL_ERROR_HANDLER;
      return TCL_OK;
error:
      POP_TCL_ERROR_HANDLER;
      double_error:
      #undef scriptError
      #define scriptError(msg) { \
        cerr << "Double-error; old message: " << scriptingErrorMessage << \
        ", new message: " << msg << endl; \
        scriptingErrorMessage = msg; goto double_error; \
      }
      if (parent) {}
if (arg0Init) {arg0Init=false; }
#undef scriptError
Tcl_SetResult(interp, scriptingErrorMessage, NULL); return TCL_ERROR; }
#undef scriptError


#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
static int
     trampoline420 (
     ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv[]) throw() {
       SHIFT;
       if (objc != 1) {
         Tcl_SetResult(interp, "Incorrect number of arguments passed to internal function", TCL_VOLATILE);
         return TCL_ERROR;
       }
       invokingInterpreter=interp;
       OldStyleExplosion* parent=NULL;
PUSH_TCL_ERROR_HANDLER(errorOccurred); if (errorOccurred) goto error;
{
      string name(Tcl_GetStringFromObj(objv[0], NULL));
      if (name != "0") {
        //Does it exist?
        InterpInfo* info=interpreters[interp];
        map<string,Export*>::iterator it=info->exportsByName.find(name);
        if (it == info->exportsByName.end()) {
          for (it=info->exportsByName.begin();
               it != info->exportsByName.end(); ++it) {
            cout << (*it).first << endl;
          }
          sprintf(staticError, "Invalid export passed to C++: %s",
                  name.c_str());
          scriptError(staticError);
        }
        Export* ex=(*it).second;
        //OK, is the type correct?
        if (ex->type->theType != typeid(OldStyleExplosion)
        &&  0==ex->type->superclasses.count(&typeid(OldStyleExplosion))) {
          //Nope
          sprintf(staticError, "Wrong type passed to C++ function; expected"
                               " OldStyleExplosion, "
                               "got %s", ex->type->tclClassName.c_str());
          scriptError(staticError);
        }

        //All is well, transfer ownership now
        OldStyleExplosion* tmp=(OldStyleExplosion*)ex->ptr;
        
        parent = tmp;
    } else parent=NULL;
}
      if (!parent) { scriptError("NULL this passed into C++"); }
try {
      
     parent->
     
     draw();

    } catch (exception& ex) {
      sprintf(staticError, "%s: %s", typeid(ex).name(), ex.what());
      scriptError(staticError);
    }
POP_TCL_ERROR_HANDLER;
      return TCL_OK;
error:
      POP_TCL_ERROR_HANDLER;
      double_error:
      #undef scriptError
      #define scriptError(msg) { \
        cerr << "Double-error; old message: " << scriptingErrorMessage << \
        ", new message: " << msg << endl; \
        scriptingErrorMessage = msg; goto double_error; \
      }
      if (parent) {}
#undef scriptError
Tcl_SetResult(interp, scriptingErrorMessage, NULL); return TCL_ERROR; }
#undef scriptError


#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
static int
     trampoline422 (
     ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv[]) throw() {
       SHIFT;
       if (objc != 1) {
         Tcl_SetResult(interp, "Incorrect number of arguments passed to internal function", TCL_VOLATILE);
         return TCL_ERROR;
       }
       invokingInterpreter=interp;
       OldStyleExplosion* parent=NULL;
float ret; Tcl_Obj* retTcl=NULL;
PUSH_TCL_ERROR_HANDLER(errorOccurred); if (errorOccurred) goto error;
{
      string name(Tcl_GetStringFromObj(objv[0], NULL));
      if (name != "0") {
        //Does it exist?
        InterpInfo* info=interpreters[interp];
        map<string,Export*>::iterator it=info->exportsByName.find(name);
        if (it == info->exportsByName.end()) {
          for (it=info->exportsByName.begin();
               it != info->exportsByName.end(); ++it) {
            cout << (*it).first << endl;
          }
          sprintf(staticError, "Invalid export passed to C++: %s",
                  name.c_str());
          scriptError(staticError);
        }
        Export* ex=(*it).second;
        //OK, is the type correct?
        if (ex->type->theType != typeid(OldStyleExplosion)
        &&  0==ex->type->superclasses.count(&typeid(OldStyleExplosion))) {
          //Nope
          sprintf(staticError, "Wrong type passed to C++ function; expected"
                               " OldStyleExplosion, "
                               "got %s", ex->type->tclClassName.c_str());
          scriptError(staticError);
        }

        //All is well, transfer ownership now
        OldStyleExplosion* tmp=(OldStyleExplosion*)ex->ptr;
        
        parent = tmp;
    } else parent=NULL;
}
      if (!parent) { scriptError("NULL this passed into C++"); }
try {
      ret =
     parent->
     
     getRadius();

    } catch (exception& ex) {
      sprintf(staticError, "%s: %s", typeid(ex).name(), ex.what());
      scriptError(staticError);
    }
{retTcl = Tcl_NewDoubleObj((double)ret);}
Tcl_SetObjResult(interp, retTcl);
POP_TCL_ERROR_HANDLER;
      return TCL_OK;
error:
      POP_TCL_ERROR_HANDLER;
      double_error:
      #undef scriptError
      #define scriptError(msg) { \
        cerr << "Double-error; old message: " << scriptingErrorMessage << \
        ", new message: " << msg << endl; \
        scriptingErrorMessage = msg; goto double_error; \
      }
      if (parent) {}
#undef scriptError
Tcl_SetResult(interp, scriptingErrorMessage, NULL); return TCL_ERROR; }
#undef scriptError

static void cppDecCode(bool safe,Tcl_Interp* interp) throw() {Tcl_CreateObjCommand(interp, "c++ trampoline416", trampoline416, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline418", trampoline418, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline420", trampoline420, 0, NULL);
Tcl_CreateObjCommand(interp, "c++ trampoline422", trampoline422, 0, NULL);
TypeExport* ste=new TypeExport(typeid(OldStyleExplosion)),
                           * ete=new TypeExport(typeid(TclOldStyleExplosion));
ste->isAObject=ete->isAObject=true;
ste->tclClassName=ete->tclClassName="OldStyleExplosion";
ste->superclasses.insert(&typeid(GameObject));
ste->superclasses.insert(&typeid(AObject));
ete->superclasses=ste->superclasses;
ete->superclasses.insert(&typeid(OldStyleExplosion));
typeExports[&typeid(OldStyleExplosion)]=ste;
typeExports[&typeid(TclOldStyleExplosion)]=ete;
}
};
void classdec415(bool safe, Tcl_Interp* interp) throw() {
  TclOldStyleExplosion::cppDecCode(safe,interp);
}

class TclStarField : public StarField {
      public:
TclStarField(GameObject* arg2, GameField* arg3, int arg4) : StarField(arg2, arg3, arg4) {
        tclExtended=invokingInterpreter;
        tclKnown=true;
      }
static StarField* constructordefault
    (const string& name, const string& magicCookie,  GameObject* arg2, GameField* arg3, int arg4) {
      InterpInfo* info=interpreters[invokingInterpreter];
      //Validate magic cookie
      if (magicCookie != info->magicCookie) {
        scriptError("Call to c++ new with invalid magic cookie!");
      }

      //Refuse if we enforce allocation and the interpreter knows too much
      if (info->enforceAllocationLimit && info->exports.size() > MAX_EXPORTS) {
        scriptError("Interpreter allocation limit exceeded");
      }

      //Refuse duplicate allocation
      if (info->exportsByName[name]) {
        scriptError("Double allocation");
      }
      StarField* ret;
      ret=new StarField(arg2, arg3, arg4);
      if (ret) {
        ret->tclKnown = true;
        //The trampoline will take care of assigning ownership to Tcl, we
        //just need to create the export so it has the correct name
        Export* ex=new Export;
        info->exports[(void*)ret]=ex;
        info->exportsByName[name]=ex;
        ex->ptr = (void*)ret;
        ex->type = typeExports[&typeid(StarField)];
        ex->interp = invokingInterpreter;
        ex->tclrep=name;
      }

      return ret;
    }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
static int
     trampoline685 (
     ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv[]) throw() {
       SHIFT;
       if (objc != 5) {
         Tcl_SetResult(interp, "Incorrect number of arguments passed to internal function", TCL_VOLATILE);
         return TCL_ERROR;
       }
       invokingInterpreter=interp;
       
string arg0; bool arg0Init=false;
string arg1; bool arg1Init=false;
GameObject* arg2; bool arg2Init=false;
GameField* arg3; bool arg3Init=false;
int arg4; bool arg4Init=false;
StarField* ret; Tcl_Obj* retTcl=NULL;
PUSH_TCL_ERROR_HANDLER(errorOccurred); if (errorOccurred) goto error;

{
      static Tcl_DString dstr;
      static bool hasDstr=false;
      if (!hasDstr) {
        Tcl_DStringInit(&dstr);
        hasDstr=true;
      } else {
        Tcl_DStringSetLength(&dstr, 0);
      }
      int length;
      Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(objv[0], &length);
      arg0 = Tcl_UniCharToUtfDString(tuc, length, &dstr);
    };
arg0Init=true;
{
      static Tcl_DString dstr;
      static bool hasDstr=false;
      if (!hasDstr) {
        Tcl_DStringInit(&dstr);
        hasDstr=true;
      } else {
        Tcl_DStringSetLength(&dstr, 0);
      }
      int length;
      Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(objv[1], &length);
      arg1 = Tcl_UniCharToUtfDString(tuc, length, &dstr);
    };
arg1Init=true;
{
      string name(Tcl_GetStringFromObj(objv[2], NULL));
      if (name != "0") {
        //Does it exist?
        InterpInfo* info=interpreters[interp];
        map<string,Export*>::iterator it=info->exportsByName.find(name);
        if (it == info->exportsByName.end()) {
          for (it=info->exportsByName.begin();
               it != info->exportsByName.end(); ++it) {
            cout << (*it).first << endl;
          }
          sprintf(staticError, "Invalid export passed to C++: %s",
                  name.c_str());
          scriptError(staticError);
        }
        Export* ex=(*it).second;
        //OK, is the type correct?
        if (ex->type->theType != typeid(GameObject)
        &&  0==ex->type->superclasses.count(&typeid(GameObject))) {
          //Nope
          sprintf(staticError, "Wrong type passed to C++ function; expected"
                               " GameObject, "
                               "got %s", ex->type->tclClassName.c_str());
          scriptError(staticError);
        }

        //All is well, transfer ownership now
        GameObject* tmp=(GameObject*)ex->ptr;
        
        arg2 = tmp;
    } else arg2=NULL;
};
arg2Init=true;
{
      string name(Tcl_GetStringFromObj(objv[3], NULL));
      if (name != "0") {
        //Does it exist?
        InterpInfo* info=interpreters[interp];
        map<string,Export*>::iterator it=info->exportsByName.find(name);
        if (it == info->exportsByName.end()) {
          for (it=info->exportsByName.begin();
               it != info->exportsByName.end(); ++it) {
            cout << (*it).first << endl;
          }
          sprintf(staticError, "Invalid export passed to C++: %s",
                  name.c_str());
          scriptError(staticError);
        }
        Export* ex=(*it).second;
        //OK, is the type correct?
        if (ex->type->theType != typeid(GameField)
        &&  0==ex->type->superclasses.count(&typeid(GameField))) {
          //Nope
          sprintf(staticError, "Wrong type passed to C++ function; expected"
                               " GameField, "
                               "got %s", ex->type->tclClassName.c_str());
          scriptError(staticError);
        }

        //All is well, transfer ownership now
        GameField* tmp=(GameField*)ex->ptr;
        
        arg3 = tmp;
    } else arg3=NULL;
};
arg3Init=true;
{int tmp;
            int err = Tcl_GetIntFromObj(interp, objv[4], &tmp);
            if (err == TCL_ERROR)
              scriptError(Tcl_GetStringResult(interp));
            arg4 = (int)tmp;};
arg4Init=true;
try {
      ret =
     
     
     constructordefault(arg0, arg1, arg2, arg3, arg4);

    } catch (exception& ex) {
      sprintf(staticError, "%s: %s", typeid(ex).name(), ex.what());
      scriptError(staticError);
    }
{if (!(ret)) {
      retTcl=Tcl_NewStringObj("0", 1);
    } else {
      InterpInfo* info=interpreters[interp];
      //Is it a valid export already?
      void* ptr=const_cast<void*>((void*)(ret));
      map<void*,Export*>::iterator it=info->exports.find(ptr);
      Export* ex;
      if (it == info->exports.end()) {
        //No, create new one
        (ret)->tclKnown=true;
        ex=new Export;
        ex->ptr=ptr;
        ex->type=typeExports[&typeid(*(ret))];
        ex->interp=interp;
        

        //Create the new Tcl-side object with
        //  new Type {}
        Tcl_Obj* cmd[3] = {
          Tcl_NewStringObj("new", 3),
          Tcl_NewStringObj(ex->type->tclClassName.c_str(),
                           ex->type->tclClassName.size()),
          Tcl_NewObj(),
        };
        for (unsigned i=0; i<lenof(cmd); ++i)
          Tcl_IncrRefCount(cmd[i]);
        int status=Tcl_EvalObjv(interp, lenof(cmd), cmd, TCL_EVAL_GLOBAL);
        for (unsigned i=0; i<lenof(cmd); ++i)
          Tcl_DecrRefCount(cmd[i]);
        if (status == TCL_ERROR) {
          sprintf(staticError, "Error exporting C++ object to Tcl: %s",
                  Tcl_GetStringResult(interp));
          scriptError(staticError);
        }

        //We can now get the name, and then have a fully-usable export
        ex->tclrep=Tcl_GetStringResult(interp);
        //Register
        info->exports[(void*)(ret)]=ex;
        info->exportsByName[ex->tclrep]=ex;
      } else {
        //Yes, use directly
        ex = (*it).second;
      }

      //Ownership
      if ((ret)) switch ((ret)->ownStat) {
            case AObject::Tcl:
              if (interp == (ret)->owner.interpreter) {
                
              }
              //fall through
            case AObject::Cpp:
              (ret)->ownStatBak=(ret)->ownStat;
              (ret)->ownerBak.interpreter = (ret)->owner.interpreter;
              (ret)->ownStat=AObject::Tcl;
              (ret)->owner.interpreter=interp;
              break;
            case AObject::Container:
              cerr <<
"FATAL: Attempt by C++ to give Tcl ownership of automatic value" << endl;
              ::exit(EXIT_PROGRAM_BUG);
          }

      //Done
      retTcl=Tcl_NewStringObj(ex->tclrep.c_str(), ex->tclrep.size());
    }}
Tcl_SetObjResult(interp, retTcl);
POP_TCL_ERROR_HANDLER;
      return TCL_OK;
error:
      POP_TCL_ERROR_HANDLER;
      double_error:
      #undef scriptError
      #define scriptError(msg) { \
        cerr << "Double-error; old message: " << scriptingErrorMessage << \
        ", new message: " << msg << endl; \
        scriptingErrorMessage = msg; goto double_error; \
      }
      
if (arg0Init) {arg0Init=false; }
if (arg1Init) {arg1Init=false; }
if (arg2Init) {arg2Init=false; }
if (arg3Init) {arg3Init=false; }
if (arg4Init) {arg4Init=false; }
#undef scriptError
Tcl_SetResult(interp, scriptingErrorMessage, NULL); return TCL_ERROR; }
#undef scriptError

static void cppDecCode(bool safe,Tcl_Interp* interp) throw() {Tcl_CreateObjCommand(interp, "c++ trampoline685", trampoline685, 0, NULL);
TypeExport* ste=new TypeExport(typeid(StarField)),
                           * ete=new TypeExport(typeid(TclStarField));
ste->isAObject=ete->isAObject=true;
ste->tclClassName=ete->tclClassName="StarField";
ste->superclasses.insert(&typeid(Background));
ste->superclasses.insert(&typeid(EffectsHandler));
ste->superclasses.insert(&typeid(AObject));
ete->superclasses=ste->superclasses;
ete->superclasses.insert(&typeid(StarField));
typeExports[&typeid(StarField)]=ste;
typeExports[&typeid(TclStarField)]=ete;
}
};
void classdec684(bool safe, Tcl_Interp* interp) throw() {
  TclStarField::cppDecCode(safe,interp);
}

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
 int
     trampoline687 (
     ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv[]) throw() {
       SHIFT;
       if (objc != 0) {
         Tcl_SetResult(interp, "Incorrect number of arguments passed to internal function", TCL_VOLATILE);
         return TCL_ERROR;
       }
       invokingInterpreter=interp;
       
PUSH_TCL_ERROR_HANDLER(errorOccurred); if (errorOccurred) goto error;

try {
      
     
     
     initStarLists();

    } catch (exception& ex) {
      sprintf(staticError, "%s: %s", typeid(ex).name(), ex.what());
      scriptError(staticError);
    }
POP_TCL_ERROR_HANDLER;
      return TCL_OK;
error:
      POP_TCL_ERROR_HANDLER;
      double_error:
      #undef scriptError
      #define scriptError(msg) { \
        cerr << "Double-error; old message: " << scriptingErrorMessage << \
        ", new message: " << msg << endl; \
        scriptingErrorMessage = msg; goto double_error; \
      }
      
#undef scriptError
Tcl_SetResult(interp, scriptingErrorMessage, NULL); return TCL_ERROR; }
#undef scriptError



#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }

#define scriptError(desc) { scriptingErrorMessage=desc; goto error; }
 int
     trampoline1033 (
     ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const objv[]) throw() {
       SHIFT;
       if (objc != 2) {
         Tcl_SetResult(interp, "Incorrect number of arguments passed to internal function", TCL_VOLATILE);
         return TCL_ERROR;
       }
       invokingInterpreter=interp;
       
unsigned arg0; bool arg0Init=false;
const char* arg1; bool arg1Init=false;
PUSH_TCL_ERROR_HANDLER(errorOccurred); if (errorOccurred) goto error;

{int tmp;
            int err = Tcl_GetIntFromObj(interp, objv[0], &tmp);
            if (err == TCL_ERROR)
              scriptError(Tcl_GetStringResult(interp));
            arg0 = (unsigned)tmp;};
arg0Init=true;
{
          static Tcl_DString dstr;
          static bool hasDstr=false;
          if (!hasDstr) {
            Tcl_DStringInit(&dstr);
            hasDstr=true;
          } else {
            Tcl_DStringSetLength(&dstr, 0);
          }
          int length;
          Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(objv[1], &length);
          arg1 = Tcl_UniCharToUtfDString(tuc, length, &dstr);
        };
arg1Init=true;
try {
      
     
     
     hud_user_messages::setmsg(arg0, arg1);

    } catch (exception& ex) {
      sprintf(staticError, "%s: %s", typeid(ex).name(), ex.what());
      scriptError(staticError);
    }
POP_TCL_ERROR_HANDLER;
      return TCL_OK;
error:
      POP_TCL_ERROR_HANDLER;
      double_error:
      #undef scriptError
      #define scriptError(msg) { \
        cerr << "Double-error; old message: " << scriptingErrorMessage << \
        ", new message: " << msg << endl; \
        scriptingErrorMessage = msg; goto double_error; \
      }
      
if (arg0Init) {arg0Init=false; }
if (arg1Init) {arg1Init=false; }
#undef scriptError
Tcl_SetResult(interp, scriptingErrorMessage, NULL); return TCL_ERROR; }
#undef scriptError



