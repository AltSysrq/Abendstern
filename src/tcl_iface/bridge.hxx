/**
 * @file
 * @author Jason Lingle
 * @brief Contains functions for dealing with the C++-Tcl bridge
 */

#ifndef BRIDGE_HXX_
#define BRIDGE_HXX_

#include <csetjmp>
#include <cstring>
#include <stack>
using std::jmp_buf;
using std::longjmp;
using std::memcpy;
using std::strlen;
//setjmp is a macro

/* Defines interfaces to the C++/Tcl glue code, such as
 * creating a new interpreter and handling scripting
 * errors.
 */
struct Tcl_Interp;
struct Tcl_Obj;

/** Prepares some internal locking mechanisms.
 * This MUST be called BEFORE any other function in this file.
 */
void prepareTclBridge();

/** Creates a new interpreter and adds all applicable
 * Abendstern bindings to it. It may be a slave or
 * a master, and may be safe or trusted.
 */
Tcl_Interp* newInterpreter(bool safe=true, Tcl_Interp* master=NULL);

/** Deletes and deregisters the given Interpreter and all its slaves.
 * All interpreter-owned C++ objects are deleted.
 */
void delInterpreter(Tcl_Interp*);

/** Like source, but works from a safe interpreter.
 * This command does not validate the path; the function
 * bool validateSafeSourcePath(const char*) should be
 * used to check that.
 */
void interpSafeSource(const char*);

/** Ensures the specified filename meets the requirements
 * of safe sourcing. Namely, it can only contain the following
 * characters:
 * \verbatim
 *   abcdefghijklmnopqrstuvwxyz_./
 * \endverbatim
 * must begin with "tcl/", and may not contain more than one '.',
 * or the sequences "/." or "./", or end with a ".".
 */
bool validateSafeSourcePath(const char*);

/* Tcl errors are handled by longjmping here, with
 * an error message set in scriptingErrorMessage.
 *
 * Since longjmp circumvents destructors in automatic
 * storage, care must be taken that no intermediate
 * C++ code beteen the setjmp and the Tcl code (either
 * direct or by Tcl overriding of virtual functions)
 * does not have any automatic variables that must
 * have their destructors run (or, if it does, it
 * must use the macros below to intercept the error
 * and run the destructors).
 *
 * Yes, this /does/ somewhat reinvent exception handling;
 * however, it allows the error handling to pass through
 * C as well, and also allows for most of Abendstern's
 * functions to be declared throw().
 */
/** Fix the nonassignablity of jmp_buf (it is an immediate array) */
struct JmpBuf {
  jmp_buf buf;

  JmpBuf() {}
  JmpBuf(const JmpBuf& other) { memcpy(&buf, &other.buf, sizeof(jmp_buf)); }
  JmpBuf(const jmp_buf& other) { memcpy(&buf, &other, sizeof(jmp_buf)); }
  operator jmp_buf&() { return buf; }
  operator jmp_buf*() { return &buf; }
  JmpBuf& operator=(const JmpBuf& other) {
    memcpy(&buf, &other.buf, sizeof(jmp_buf));
    return *this;
  }
  JmpBuf& operator=(const jmp_buf& other) {
    memcpy(&buf, &other, sizeof(jmp_buf));
    return *this;
  }
};
/** Pointer to current destination for Tcl errors */
extern THREAD_LOCAL JmpBuf* currentTclErrorHandler_ptr;

/** Stack of Tcl error destinations */
extern THREAD_LOCAL std::stack<JmpBuf>* tclErrorHandlerStack_ptr;

/** Automatic dereference */
#define tclErrorHandlerStack (*tclErrorHandlerStack_ptr)
/** Automatic dereference */
#define currentTclErrorHandler (*currentTclErrorHandler_ptr)

/** Initialises thread-local storage. This MUST be called EXACTLY
 * once in a thread BEFORE the construction of an interpreter.
 */
void initBridgeTLS();

/** The PUSH macro saves the result of setjmp in the variable
 * passed to it (it declares it).
 */
#define PUSH_TCL_ERROR_HANDLER(ret)\
  tclErrorHandlerStack.push(currentTclErrorHandler); \
  int ret=setjmp(currentTclErrorHandler)
/** The POP macro undoes a PUSH. */
#define POP_TCL_ERROR_HANDLER currentTclErrorHandler=tclErrorHandlerStack.top(); tclErrorHandlerStack.pop()

/** Automatically protect all automatic variables in
 * the current scope (only above). To work correctly,
 * the form MUST be as follows:
 * \verbatim
 *   (new scope) {
 *     VariableThatNeedsToBeProtected foo;
 *     UNWIND_PROTECT;
 *     ...
 *     end_of_scope: (possible cleanup code);
 *   }
 *   UNWIND_PROTECT_END;
 * \endverbatim
 * It is IMPERATIVE that no Tcl code could possibly
 * be called between end_of_scope and UNWIND_PROTECT_END.
 */
#define UNWIND_PROTECT \
  tclErrorHandlerStack.push(currentTclErrorHandler); \
  if (setjmp(currentTclErrorHandler)) goto end_of_scope
/** @see UNWIND_PROTECT */
#define UNWIND_PROTECT_END \
  currentTclErrorHandler=tclErrorHandlerStack.top(); \
  tclErrorHandlerStack.pop(); \
  longjmp(currentTclErrorHandler, 1)

/** If setjmp returned non-zero, this contains an error
 * description.
 */
extern THREAD_LOCAL const char* scriptingErrorMessage;

/** Indicates that the given foreign object is ceasing to exist or
 * exiting Abendstern's control, so any Tcl exports to it are now
 * invalid.
 */
void foreignLoseControl(void*);

/** The most recent C++-invoking Tcl interpreter on the current thread */
extern THREAD_LOCAL Tcl_Interp* invokingInterpreter;

#endif /*BRIDGE_HXX_*/
