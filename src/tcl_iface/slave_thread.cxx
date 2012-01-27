/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/tcl_iface/slave_thread.hxx
 */

#include <deque>
#include <string>
#include <cstring>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <cassert>
#include <csetjmp>

#include <SDL.h>
#include <SDL_thread.h>

#include <tcl.h>

#include "slave_thread.hxx"
#include "src/exit_conditions.hxx"

using namespace std;

static SDL_Thread* thread;
static SDL_mutex* flock,* block;
static SDL_cond* cond;

static volatile bool requestTerminate;

struct message {
  string txt;
  string name;
};
static deque<message> fgq, bgq;

static jmp_buf jmpbuf;

/* It seems that [incr Tcl] is not thread-safe on Windows (at the very
 * least, using the normal Abendstern C++/Tcl bridge obliterates the
 * stack, and it looks like Itcl is to blame).
 * The background thread only needs three functions, though, so we'll
 * just hand-implement them here.
 *
 * Since the background thread is completely trusted, we can also make
 * some assumptions.
 */
static int callBkgAns(ClientData, Tcl_Interp*, int objc, Tcl_Obj*const* objv) {
  assert(objc == 2);
  static Tcl_DString dstr;
  static bool hasdstr = false;
  if (!hasdstr) {
    hasdstr = true;
    Tcl_DStringInit(&dstr);
  }
  int length;
  Tcl_UniChar* tuc = Tcl_GetUnicodeFromObj(objv[1],&length);
  bkg_ans(Tcl_UniCharToUtfDString(tuc, length, &dstr));
  return TCL_OK;
}

static int callBkgRcv(ClientData, Tcl_Interp* interp, int objc, Tcl_Obj*const* objv) {
  static Tcl_DString dstr;
  static bool hasDstr=false;
  if (!hasDstr) {
    hasDstr = true;
    Tcl_DStringInit(&dstr);
  }

  const char* str = bkg_rcv();
  Tcl_UniChar* tuc = Tcl_UtfToUniCharDString(str? str : "", -1, &dstr);
  Tcl_SetObjResult(interp, Tcl_NewUnicodeObj(tuc, -1));
  return TCL_OK;
}

static int callBkgWait(ClientData, Tcl_Interp*, int, Tcl_Obj*const*) {
  bkg_wait();
  return TCL_OK;
}

static int run(void*) {
  requestTerminate = false;
  if (setjmp(jmpbuf)) return 0; //Used to terminate the thread

  Tcl_Interp* interp = Tcl_CreateInterp();
  Tcl_Init(interp);
  Tcl_CreateObjCommand(interp, "bkg_ans", callBkgAns, 0, NULL);
  Tcl_CreateObjCommand(interp, "bkg_rcv", callBkgRcv, 0, NULL);
  Tcl_CreateObjCommand(interp, "bkg_wait", callBkgWait, 0, NULL);
  if (TCL_ERROR == Tcl_EvalFile(interp, "tcl/bkginit.tcl")) {
    cerr << "FATAL: Background thread died: " << Tcl_GetStringResult(interp) << endl;
    Tcl_Obj* options=Tcl_GetReturnOptions(interp, TCL_ERROR);
    Tcl_Obj* key=Tcl_NewStringObj("-errorinfo", -1);
    Tcl_Obj* stackTrace;
    Tcl_IncrRefCount(key);
    Tcl_DictObjGet(NULL, options, key, &stackTrace);
    Tcl_DecrRefCount(key);
    cerr << "Stack trace:\n" << Tcl_GetStringFromObj(stackTrace, NULL) << endl;
    exit(EXIT_SCRIPTING_BUG);
  }
  return 0;
}

void bkg_start() {
  flock = SDL_CreateMutex();
  block = SDL_CreateMutex();
  cond = SDL_CreateCond();
  thread = SDL_CreateThread(run, NULL);
}

void bkg_term() {
  requestTerminate = true;
  //SDL_mutexP(block);
  SDL_CondBroadcast(cond);
  SDL_WaitThread(thread, NULL);
  //SDL_mutexV(block);
}

void bkg_req(const char* txt, const char* name) {
  //We need to send this BEFORE acquiring the lock, as a
  //waiting thread will already have the lock
  SDL_CondBroadcast(cond);

  SDL_mutexP(block);

  if (*name) {
    //Remove anything with this name
    for (deque<message>::iterator it = bgq.begin(); it != bgq.end(); ++it)
      if (it->name == name)
        it = bgq.erase(it)-1;
  }

  message msg;
  msg.txt = txt;
  msg.name = name;

  bgq.push_back(msg);

  SDL_mutexV(block);
}

const char* bkg_rcv() {
  if (requestTerminate) longjmp(jmpbuf,1);
  SDL_mutexP(block);

  static string str;
  if (bgq.empty())
    str = "";
  else {
    str = bgq.front().txt;
    bgq.pop_front();
  }

  SDL_mutexV(block);
  return str.c_str();
}

void bkg_ans(const char* txt, const char* name) {
  SDL_mutexP(flock);

  if (*name)
    for (deque<message>::iterator it = fgq.begin(); it != fgq.end(); ++it)
      if (it->name == name)
        it = fgq.erase(it)-1;

  message msg;
  msg.txt = txt;
  msg.name = name;

  fgq.push_back(msg);

  SDL_mutexV(flock);
}

const char* bkg_get() {
  SDL_mutexP(flock);

  static string str;
  if (fgq.empty())
    str = "";
  else {
    str = fgq.front().txt;
    fgq.pop_front();
  }

  SDL_mutexV(flock);
  return str.c_str();
}

void bkg_wait() {
  if (requestTerminate) longjmp(jmpbuf,1);
  SDL_mutexP(block);
  if (bgq.empty())
    SDL_CondWait(cond, block);
  SDL_mutexV(block);
}
