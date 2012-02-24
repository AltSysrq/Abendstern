/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.23
 * @brief Exposes implemementation interfaces for the C++/Tcl bridge.
 *
 * Note: Documentation can be found in the implementation file. This header
 * only exists separately since multiple generated files use it.
 */

#ifndef IMPLEMENTATION_HXX_
#define IMPLEMENTATION_HXX_

#include <stack>
#include <typeinfo>
#include <set>
#include <map>
#include <csetjmp>

#include <tcl.h>
#include <SDL.h>

#include "bridge.hxx"

#ifndef DOXYGEN
void scriptError(const char*);
namespace tcl_glue_implementation {
  extern SDL_mutex* mutex;
  void prepareTclBridge();
  #define Tcl_SetResult(interp, str, ignore) \
    Tcl_SetObjResult(interp, Tcl_NewStringObj(str, strlen(str)+1))

  extern THREAD_LOCAL char staticError[1024];


  /* Defines an exported type interface.
   * A map from function names to static forwarders, as well
   * as a set of things that it can be cast to.
   */
  struct TypeExport {
    bool isAObject;
    const std::type_info& theType;
    std::set<const std::type_info*> superclasses;
    std::string tclClassName;

    TypeExport(const std::type_info& t) : theType(t) {}
  };

  extern THREAD_LOCAL std::map<const std::type_info*, TypeExport*>*
      typeExports_ptr;
  #define typeExports (*tcl_glue_implementation::typeExports_ptr)

  /* Represents an exported object.
   * This struct stores the pointer to the object,
   * a pointer to its type definitions, and the
   * interpreter to which this instance is exported.
   *
   * These Exports are stored via pointers in Tcl_Objs.
   * Their string representation is the name of the
   * object they are bound to. They can only enter
   * Tcl in two ways: By importation (C++ object passed
   * into or returned to Tcl) and via an explicit request
   * to allocate an object. These requests require a
   * "magic cookie" to ensure that the object will be controlled
   * solely by trusted code; only the topmost class of all
   * the Tcl glue code knows this cookie (it is dropped
   * into the code at eval-time and then discarded).
   * Deallocation of an Export* occurs not in the Tcl "free"
   * binding (in the Tcl_ObjType), but by a call to
   * c++_deexport. Since any code has access to the
   * Export, this also requires the magic cookie.
   */
  struct Export {
    void* ptr;
    TypeExport* type;
    //We need to store this here, since Tcl does not
    //pass itself to all functions
    Tcl_Interp* interp;
    //Store this too so we can quickly create the
    //string representation
    std::string tclrep;
  };

  /* This struct defines interpreter-specific import/export
   * information.
   */
  struct InterpInfo {
    Tcl_Interp* interpreter;
    /* If true, we sort-of forbid the interpreter from having
     * more than 4096 exports. "Sort-of" because we only check
     * it when a new C++ object is allocated from within Tcl.
     * However, it is those Tcl-allocated objects that /could/
     * be used to leak memory, so that should be good enough.
     */
    #define MAX_EXPORTS 4096
    bool enforceAllocationLimit;

    /* A map to quickly find the Export representing
     * a pointer.
     */
    std::map<void*, Export*> exports;
    /* Similar, but map from Tcl names to Exports.
     */
    std::map<std::string, Export*> exportsByName;

    /* The top-secret key used for permission to allocate objects. */
    char magicCookie[32];

    std::string slaveName;
  }; //struct InterpInfo
  //map<Tcl_Interp*, InterpInfo*> interpreters;
  //This class exists to allow ~AObject to detect that it is being
  //run with the global destructors, and thus interpreters is no
  //longer a valid construct
  class Interpreters: public std::map<Tcl_Interp*, InterpInfo*> {
  public:
    bool destroyed;
    Interpreters() : map<Tcl_Interp*,InterpInfo*>(), destroyed(false) {}
    ~Interpreters() { destroyed=true; }
  };

  extern THREAD_LOCAL Interpreters* interpreters_ptr;
  #define interpreters (*tcl_glue_implementation::interpreters_ptr)

  Tcl_Interp* newInterpreterImpl(bool, Tcl_Interp*);
  void newInterpreterImplPost(Tcl_Interp*);
  void cppDelete(const std::string&, const std::string&);
  void debugTclExports();

  #ifdef DEBUG
  void commitSuicideInsteadOfLongJump(jmp_buf,int);
  #endif
}

#endif /* DOXYGEN */
#endif /* IMPLEMENTATION_HXX_ */
