/**
 * @file
 * @author Jason Lingle
 * @brief This file provides a series of templates (DynFun0...DynFun9) that
 * allow Tcl to "create" C++ function pointers.
 *
 * This is accomplished
 * by a table of 16 function objects (extended by Tcl) and 1024
 * corresponding functions.
 *
 * THIS FILE IS RECURSIVE, ie, it includes itself.
 * I don't think it is safe to include it outside of include_all_headers.hxx.
 */

#ifndef DOXYGEN /* Doxygen: You are not expected to understand this. */
#ifndef DYNFUN_HXX_MAIN_
#define DYNFUN_HXX_MAIN_

#include <cstring>
#include <stdexcept>
#include <exception>
#include "src/core/aobject.hxx"
#include "bridge.hxx"

#define DYNFUN_TABLE_SZ 16
#define CURR_ARG
#define GLUE_(x,y) x##y
#define GLUE(x,y) GLUE_(x,y)
#define ARITY 0
//If we use __FILE__, GCC uses "../tcl_iface/dynfun.hxx", which
//causes a too-long path to be created
#define FILE "dynfun.hxx"

#include FILE
#else /* defined(DYNFUN_HXX_MAIN_) */

#if defined(DECLARE_TEMPLATE_ARGUMENTS)
  #undef DECLARE_TEMPLATE_ARGUMENTS
  #if CURR_ARG < ARITY
    ,
    #define GET_ARG_NAMES
    #include FILE
    typename ARG_TYPE
    #define NEXT_ARG
    #include FILE
    #define DECLARE_TEMPLATE_ARGUMENTS
    #include FILE
  #endif /* CURR_ARG < ARITY */
#elif defined(LIST_TEMPLATE_ARGUMENTS)
  #undef LIST_TEMPLATE_ARGUMENTS
  #if CURR_ARG < ARITY
    ,
    #define GET_ARG_NAMES
    #include FILE
    ARG_TYPE
    #define NEXT_ARG
    #include FILE
    #define LIST_TEMPLATE_ARGUMENTS
    #include FILE
  #endif /* CURR_ARG < ARITY */
#elif defined(DECLARE_FUNCTION_ARGUMENTS)
  #undef DECLARE_FUNCTION_ARGUMENTS
  #if CURR_ARG < ARITY
    #if CURR_ARG != 0
      ,
    #endif /* CURR_ARG != 0 */
    #define GET_ARG_NAMES
    #include FILE
    ARG_TYPE ARG_NAME
    #define NEXT_ARG
    #include FILE
    #define DECLARE_FUNCTION_ARGUMENTS
    #include FILE
  #endif /* CURR_ARG < ARITY */
#elif defined(LIST_FUNCTION_ARGUMENTS)
  #undef LIST_FUNCTION_ARGUMENTS
  #if CURR_ARG < ARITY
    #if CURR_ARG != 0
      ,
    #endif /* CURR_ARG != 0 */
    #define GET_ARG_NAMES
    #include FILE
    ARG_NAME
    #define NEXT_ARG
    #include FILE
    #define LIST_FUNCTION_ARGUMENTS
    #include FILE
  #endif /* CURR_ARG < ARITY */
#elif defined(GET_ARG_NAMES)
  #undef GET_ARG_NAMES
  #ifdef ARG_TYPE
    #undef ARG_TYPE
    #undef ARG_NAME
  #endif
  #if CURR_ARG == 0
    #define ARG_TYPE A
    #define ARG_NAME a
  #elif CURR_ARG == 1
    #define ARG_TYPE B
    #define ARG_NAME b
  #elif CURR_ARG == 2
    #define ARG_TYPE C
    #define ARG_NAME c
  #elif CURR_ARG == 2
    #define ARG_TYPE C
    #define ARG_NAME c
  #elif CURR_ARG == 3
    #define ARG_TYPE D
    #define ARG_NAME d
  #elif CURR_ARG == 4
    #define ARG_TYPE E
    #define ARG_NAME e
  #elif CURR_ARG == 5
    #define ARG_TYPE F
    #define ARG_NAME f
  #elif CURR_ARG == 6
    #define ARG_TYPE G
    #define ARG_NAME g
  #elif CURR_ARG == 7
    #define ARG_TYPE H
    #define ARG_NAME h
  #elif CURR_ARG == 8
    #define ARG_TYPE I
    #define ARG_NAME i
  #elif CURR_ARG == 9
    #define ARG_TYPE J
    #define ARG_NAME j
  #else
    #error Bad agrument index
  #endif
#elif defined(NEXT_ARG)
  #undef NEXT_ARG
  #if CURR_ARG == 0
    #undef CURR_ARG
    #define CURR_ARG 1
  #elif CURR_ARG == 1
    #undef CURR_ARG
    #define CURR_ARG 2
  #elif CURR_ARG == 2
    #undef CURR_ARG
    #define CURR_ARG 3
  #elif CURR_ARG == 3
    #undef CURR_ARG
    #define CURR_ARG 4
  #elif CURR_ARG == 4
    #undef CURR_ARG
    #define CURR_ARG 5
  #elif CURR_ARG == 5
    #undef CURR_ARG
    #define CURR_ARG 6
  #elif CURR_ARG == 6
    #undef CURR_ARG
    #define CURR_ARG 7
  #elif CURR_ARG == 7
    #undef CURR_ARG
    #define CURR_ARG 8
  #elif CURR_ARG == 8
    #undef CURR_ARG
    #define CURR_ARG 9
  #else
    #error Bad argument index
  #endif
#elif defined(NEXT_ARITY)
  #undef NEXT_ARITY
  #if ARITY == 0
    #undef ARITY
    #define ARITY 1
  #elif ARITY == 1
    #undef ARITY
    #define ARITY 2
  #elif ARITY == 2
    #undef ARITY
    #define ARITY 3
  #elif ARITY == 3
    #undef ARITY
    #define ARITY 4
  #elif ARITY == 4
    #undef ARITY
    #define ARITY 5
  #elif ARITY == 5
    #undef ARITY
    #define ARITY 6
  #elif ARITY == 6
    #undef ARITY
    #define ARITY 7
  #elif ARITY == 7
    #undef ARITY
    #define ARITY 8
  #elif ARITY == 8
    #undef ARITY
    #define ARITY 9
  #elif ARITY == 9
    #undef ARITY
    #define ARITY 10
  #else
    #error Bad arity
  #endif
#elif defined(CREATE_TRAMPOLINES)
  #undef CREATE_TRAMPOLINES
  #include "dynfun_create_trampolines.hxx"
#elif defined(LIST_TRAMPOLINES)
  #undef LIST_TRAMPOLINES
  #include "dynfun_list_trampolines.hxx"
#else
template <typename R
#undef CURR_ARG
#define CURR_ARG 0
#define DECLARE_TEMPLATE_ARGUMENTS
#include FILE
>
class GLUE(DynFun,ARITY): public AObject {
  public:
  typedef R (fun_t)(
    #undef CURR_ARG
    #define CURR_ARG 0
    #define DECLARE_FUNCTION_ARGUMENTS
    #include FILE
  );
  typedef GLUE(DynFun,ARITY)<R
    #undef CURR_ARG
    #define CURR_ARG 0
    #define LIST_TEMPLATE_ARGUMENTS
    #include FILE
  > This;

  /* This is ridiculously stupid...
   * If I just leave these as
   *   static This* impls[DYNFUN_TABLE_SZ],
   * I get "no matching symbol ... "
   * According to the Internet, one MUST initialize
   * static data members in a template, or they don't exist.
   * But if I do
   *   static This** impls = NULL;
   * I get
   *   "invalid in-class initialization of static data member of non-integral type ..."
   * Fine, GCC (and MSVC++). Have your initialized integers.
   * OK, I can't even initialize THESE in the class. But apparently the below IS legal,
   * outside the template (?!):
   *   template<...>
   *   unsigned long DynFun#<...>::implsInt=0;
   */
  static unsigned long implsInt;
  #define impls (reinterpret_cast<This**>(implsInt))
  #define tramps (reinterpret_cast<fun_t**>(trampsInt))
  #define CREATE_TRAMPOLINES
  #include FILE
  static unsigned long trampsInt;

  unsigned slotNum;
  GLUE(DynFun,ARITY)() {
    if (!impls) {
      implsInt = reinterpret_cast<unsigned long>(new This*[DYNFUN_TABLE_SZ]);
      trampsInt = reinterpret_cast<unsigned long>(new fun_t*[DYNFUN_TABLE_SZ]);
      memset(impls, 0, sizeof(void*)*DYNFUN_TABLE_SZ);
      fun_t* trmps[DYNFUN_TABLE_SZ] = {
        #define LIST_TRAMPOLINES
        #include FILE
      };
      memcpy(tramps, trmps, sizeof(void*)*DYNFUN_TABLE_SZ);
    }
    //Find first open slot
    bool slotFound=false;
    for (unsigned i=0; i<DYNFUN_TABLE_SZ && !slotFound; ++i) {
      if (!impls[i]) {
        slotNum=i;
        impls[i]=this;
        slotFound=true;
      }
    }
    if (!slotFound) {
      throw std::runtime_error("Out of slots");
    }
  }

  virtual ~GLUE(DynFun,ARITY)() {
    impls[slotNum]=NULL;
    foreignLoseControl((void*)tramps[slotNum]);
  }

  virtual R invoke(
    #undef CURR_ARG
    #define CURR_ARG 0
    #define DECLARE_FUNCTION_ARGUMENTS
    #include FILE
  ) = 0;

  static R call(fun_t* fun
    #if ARITY > 0
      ,
    #endif
    #define DECLARE_FUNCTION_ARGUMENTS
    #undef CURR_ARG
    #define CURR_ARG 0
    #include FILE
  ) {
    return fun(
      #undef CURR_ARG
      #define CURR_ARG 0
      #define LIST_FUNCTION_ARGUMENTS
      #include FILE
    );
  }

  fun_t* get() const { return tramps[slotNum]; }
  #undef tramps
  #undef impls
};

template<typename R
#undef CURR_ARG
#define CURR_ARG 0
#define DECLARE_TEMPLATE_ARGUMENTS
#include FILE
> unsigned long GLUE(DynFun,ARITY)<R
#undef CURR_ARG
#define CURR_ARG 0
#define LIST_TEMPLATE_ARGUMENTS
#include FILE
>::implsInt = 0;
template<typename R
#undef CURR_ARG
#define CURR_ARG 0
#define DECLARE_TEMPLATE_ARGUMENTS
#include FILE
> unsigned long GLUE(DynFun,ARITY)<R
#undef CURR_ARG
#define CURR_ARG 0
#define LIST_TEMPLATE_ARGUMENTS
#include FILE
>::trampsInt = 0;

#define NEXT_ARITY
#include FILE
#if ARITY <= 9
  #include FILE
#endif /* ARITY <= 9 */
#endif /* various defined(...) */
#endif /* DYNFUN_HXX_MAIN_ */
#endif /* DOXYGEN */
