/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genetic_ai.hxx
 */

/*
 * genetic_ai.cxx
 *
 *  Created on: 17.10.2011
 *      Author: jason
 */

#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <map>
#include <algorithm>
#include <typeinfo>
#include <cassert>
#include <ctime>

#if !defined(WIN32) && defined(DEBUG)
#include </usr/include/fenv.h>
#endif

#include <libconfig.h++>

#include "genetic_ai.hxx"
#include "src/ship/everything.hxx"
#include "src/globals.hxx"
#include "src/sim/blast.hxx"

using namespace std;
using namespace libconfig;

//These costs were calculated using the calculateGeneticAIFunctionCosts() function
#define FUN_COST_ADD    100
#define FUN_COST_SUB    100
#define FUN_COST_MUL    100
#define FUN_COST_DIV    100
#define FUN_COST_MOD    200
#define FUN_COST_EXP    180
#define FUN_COST_ABS    58
#define FUN_COST_NEG    72
#define FUN_COST_NOT    112
#define FUN_COST_EQ     123
#define FUN_COST_GT     123
#define FUN_COST_LT     123
#define FUN_COST_NEQ    123
#define FUN_COST_LEQ    123
#define FUN_COST_GEQ    123
#define FUN_COST_COS    238
#define FUN_COST_SIN    238
#define FUN_COST_TAN    309
#define FUN_COST_ACOS   120
#define FUN_COST_ASIN   150
#define FUN_COST_ATAN   234
#define FUN_COST_ATAN2  297
#define FUN_COST_SQRT   156
#define FUN_COST_LN     87
#define FUN_COST_ORL    50
#define FUN_COST_ANL    50
#define FUN_COST_IF     50

#define TICKS_PER_MS    1024

//The maximum stack size supported, and conversely
//the limit on function complexity (the latter is conservative:
//each term cannot push more than one value, so just limit
//the number of terms to this value)
#define MAX_STACK_SIZE 256

typedef GeneticAI::evaluator_fun evalf;

//Converts the given float to a bool using the rules defined in the design
static inline bool f2b(float f) { return f > 0; }
//Converts the given bool to a float
static inline float b2f(bool b) { return b? 1.0f : 0.0f; }
//Converts the given float to a ranged unsigned
static inline unsigned f2u(float f, unsigned maxex) {
  signed s = (signed)max(-9999.0f, min(+9999.0f,f));
  s %= (signed)maxex;
  //Who would ever want a negative out of a mod?
  //Seriously, why do both % and fmod use this behaviour?
  if (s < 0) s += maxex;
  return (unsigned)s;
}

//Evaluates a section of evalfs, passing the given arguments.
static void runEvalfs(const evalf*& begin, const evalf* end,
                      float*& stack, unsigned& cost,
                      const float*& fparms, const unsigned*& iparms) {
  while (begin < end) {
    evalf f = *begin++;
    f(stack, fparms, iparms, cost, (const void*const*&)begin);
  }
}

/*-**************************************************
 * BEGIN: FUNCTIONS                                 *
 ****************************************************/
namespace {
extern void f_add(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  *stack += a;
  cost += FUN_COST_ADD;
}

extern void f_sub(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  *stack -= a;
  cost += FUN_COST_SUB;
}

extern void f_mul(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  *stack *= a;
  cost += FUN_COST_MUL;
}

extern void f_div(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  if (a == +0.0f || a == -0.0f) {
    *stack = 0;
  } else {
    *stack /= a;
  }

  cost += FUN_COST_DIV;
}

extern void f_mod(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  if (a == +0.0f || a == -0.0f) {
    *stack = 0;
  } else {
    float b = fmod(*stack,a);
    if (b == -0.0f) b = +0.0f;
    else if (b < -0.0f) b += a;
    *stack = b;
  }
  cost += FUN_COST_MOD;
}

extern void f_exp(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float power = *stack--;
  float base = *stack;
  if (base <= +0.0f) {
    //Undefined result (except then power is an integer, but
    //we won't handle those cases, which will be rare)
    *stack = 1;
  } else {
    *stack = pow(base,power);
  }
  cost += FUN_COST_EXP;
}

extern void f_abs(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  if (*stack < +0.0f) *stack = -*stack;
  cost += FUN_COST_ABS;
}

extern void f_neg(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  *stack = -*stack;
  cost += FUN_COST_NEG;
}

extern void f_not(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  *stack = b2f(!f2b(*stack));
  cost += FUN_COST_NOT;
}

//Equals will be approximate
extern void f_equ(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  float b = *stack;
  *stack = b2f(a >= b*0.95f && b >= a*0.95f);
  cost += FUN_COST_EQ;
}

extern void f_lt (float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  *stack = b2f(*stack < a);
  cost += FUN_COST_LT;
}

extern void f_gt (float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  *stack = b2f(*stack > a);
  cost += FUN_COST_GT;
}

extern void f_neq(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  float b = *stack;
  *stack = b2f(a < b*0.95f || b > a*0.95f);
  cost += FUN_COST_NEQ;
}

extern void f_leq(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  *stack = b2f(*stack <= a);
  cost += FUN_COST_LEQ;
}

extern void f_geq(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack--;
  *stack = b2f(*stack >= a);
  cost += FUN_COST_GEQ;
}

extern void f_cos(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  *stack = cos(*stack);
  cost += FUN_COST_COS;
}

extern void f_sin(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  *stack = sin(*stack);
  cost += FUN_COST_SIN;
}

extern void f_tan(float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  *stack = tan(*stack);
  cost += FUN_COST_TAN;
}

extern void f_acos(float*& stack, const float*&, const unsigned*&,
                   unsigned& cost, const void*const*&) {
  float a = *stack;
  if (a < -1.0f || a > 1.0f) {
    *stack = 0;
  } else {
    *stack = acos(a);
  }
  cost += FUN_COST_ACOS;
}

extern void f_asin(float*& stack, const float*&, const unsigned*&,
                   unsigned& cost, const void*const*&) {
  float a = *stack;
  if (a < -1.0f || a > 1.0f) {
    *stack = 0;
  } else {
    *stack = asin(a);
  }
  cost += FUN_COST_ASIN;
}

extern void f_atan(float*& stack, const float*&, const unsigned*&,
                   unsigned& cost, const void*const*&) {
  *stack = atan(*stack);
  cost += FUN_COST_ATAN;
}

extern void f_atan2(float*& stack, const float*&, const unsigned*&,
                    unsigned& cost, const void*const*&) {
  float x = *stack--;
  float y = *stack;
  if ((x == -0.0f || x == +0.0f)
  &&  (y == -0.0f || y == +0.0f)) {
    *stack = 0; //Undefined
  } else {
    *stack = atan2(y,x);
  }

  cost += FUN_COST_ATAN2;
}

extern void f_sqrt(float*& stack, const float*&, const unsigned*&,
                   unsigned& cost, const void*const*&) {
  float a = *stack;
  if (a < +0.0f)
    *stack = 0;
  else
    *stack = sqrt(a);

  cost += FUN_COST_SQRT;
}

extern void f_ln (float*& stack, const float*&, const unsigned*&,
                  unsigned& cost, const void*const*&) {
  float a = *stack;
  if (a <= +0.0f)
    *stack = 0;
  else
    *stack = log(a);
  cost += FUN_COST_LN;
}

//If pops a value from the stack; if it is true, takes an int from
//iparms, evaluates that many evalfs, then takes another iparm and
//skips that many; if false, skips then executes.
extern void f_if(float*& stack, const float*& fparms, const unsigned*& iparms,
                 unsigned& cost, const void*const*& evalfs_void) {
  float a = *stack--;
  unsigned trueLen = *iparms++, falseLen = *iparms++,
           trueFLen = *iparms++, falseFLen = *iparms++,
           trueILen = *iparms++, falseILen = *iparms++;
  const evalf*& evalfs=((const evalf*&)evalfs_void);
  if (f2b(a)) {
    runEvalfs(evalfs, evalfs+trueLen, stack, cost, fparms, iparms);
    evalfs += falseLen;
    iparms += falseILen;
    fparms += falseFLen;
  } else {
    evalfs += trueLen;
    iparms += trueILen;
    fparms += trueFLen;
    runEvalfs(evalfs, evalfs+falseLen, stack, cost, fparms, iparms);
  }
  cost += FUN_COST_IF;
}

//AND reads a value from the stack; if it is false, it skips the number of
//evalfs in an iparm; otherwise, it pops from the stack and evals that many
//evalfs
extern void f_anl(float*& stack, const float*& fparms, const unsigned*& iparms,
                  unsigned& cost, const void*const*& evalfs_void) {
  const evalf*& evalfs=((const evalf*&)evalfs_void);
  unsigned len = *iparms++, flen = *iparms++, ilen = *iparms++;
  if (!f2b(*stack)) { //False, skip
    evalfs += len;
    fparms += flen;
    iparms += ilen;
  } else {
    --stack;
    runEvalfs(evalfs, evalfs+len, stack, cost, fparms, iparms);
  }
  cost += FUN_COST_ANL;
}

//OR works basically the same way
extern void f_orl(float*& stack, const float*& fparms, const unsigned*& iparms,
                  unsigned& cost, const void*const*& evalfs_void) {
  const evalf*& evalfs=((const evalf*&)evalfs_void);
  unsigned len = *iparms++, flen = *iparms++, ilen = *iparms++;
  if (f2b(*stack)) { //True, skip
    evalfs += len;
    fparms += flen;
    iparms += ilen;
  } else {
    --stack;
    runEvalfs(evalfs, evalfs+len, stack, cost, fparms, iparms);
  }
  cost += FUN_COST_ORL;
}
} //End anonymous namespace for static-like effect with extern

/*-***************************************************************************
 * BEGIN: NON-FUNCTIONS                                                      *
 *****************************************************************************/

//Reads a constant off the fparms and pushes it onto the stack
static void v_const(float*& stack, const float*& fparms, const unsigned*&,
                    unsigned&, const void*const*&) {
  *++stack = *fparms++;
}

//Reads a float* from evalfs, dereferences it, and pushes the value onto the stack.
static void v_input(float*& stack, const float*&, const unsigned*&,
                    unsigned&, const void*const*& evalfs) {
  *++stack = *(const float*)*evalfs++;
}

//The noise pseudoinput
//Why do template arguments NEED external linkage?
//That makes no sense, especially since C++03 requires the implementation
//to be instantiated in the same translation unit anyway
namespace {
extern void v_noise(float*& stack, const float*&, const unsigned*&,
                    unsigned&, const void*const*&) {
  *++stack = (rand()%RAND_MAX)/(float)RAND_MAX;
}
}

/*-***************************************************************************
 * BEGIN: EXPRESSION COMPILATION                                             *
 *****************************************************************************/

/* Compilation is performed by simply calling compiler_functions on the
 * various Settings, which encode the data appropriately into the given
 * vectors.
 * Returns true if successful.
 */
static bool compile_subexpr(const Setting&,
                            vector<evalf>&,
                            vector<float>&,
                            vector<unsigned>&,
                            const float*);
typedef bool (*compiler_function)(const Setting&,
                                  vector<evalf>&,
                                  vector<float>&,
                                  vector<unsigned>&,
                                  const float* inputArray);

//Encodes a simple function call
template<evalf F, unsigned Arity>
static bool compile_fun(const Setting& s, vector<evalf>& evals,
                        vector<float>& fp, vector<unsigned> &ip,
                        const float* ia) {
  if (s.getLength() != Arity+1) return false;
  for (unsigned i=1; i <= Arity; ++i)
    if (!compile_subexpr(s[i], evals, fp, ip, ia))
      return false;
  evals.push_back(F);

  return true;
}

//Encodes a simple variable access
template<GeneticAI::InputOutput VarIndex>
static bool compile_var(const Setting&, vector<evalf>& evals,
                        vector<float>&, vector<unsigned>&,
                        const float* ia) {
  evals.push_back(v_input);
  evals.push_back(reinterpret_cast<evalf>(ia+(unsigned)VarIndex));

  return true;
}

//Encodes a floating-point constant
static bool compile_const(const Setting& s, vector<evalf>& evals,
                          vector<float>& fp, vector<unsigned>&,
                          const float*) {
  evals.push_back(v_const);
  fp.push_back(s);

  return true;
}

//Encodes the if function
static bool compile_if(const Setting& s, vector<evalf>& evals,
                       vector<float>& fp, vector<unsigned>& ip,
                       const float* ia) {
  if (s.getLength() != 4) return false;

  //Condition must always be evaluated, so do it normally
  if (!compile_subexpr(s[1], evals, fp, ip, ia)) return false;

  //Compile the two cases into separate vectors so we can insert
  //the data we need easily
  vector<evalf> te, fe;
  vector<float> tf, ff;
  vector<unsigned> ti, fi;
  if (!compile_subexpr(s[2], te, tf, ti, ia)) return false;
  if (!compile_subexpr(s[3], fe, ff, fi, ia)) return false;

  //Add our data
  evals.push_back(f_if);
  ip.push_back(te.size());
  ip.push_back(fe.size());
  ip.push_back(tf.size());
  ip.push_back(ff.size());
  ip.push_back(ti.size());
  ip.push_back(fi.size());

  //Append sub data
  evals.insert(evals.end(), te.begin(), te.end());
  evals.insert(evals.end(), fe.begin(), fe.end());
  ip.insert(ip.end(), ti.begin(), ti.end());
  ip.insert(ip.end(), fi.begin(), fi.end());
  fp.insert(fp.end(), tf.begin(), tf.end());
  fp.insert(fp.end(), ff.begin(), ff.end());

  return true;
}

//Encodes the two logical operators
template<evalf F>
static bool compile_logic(const Setting& s, vector<evalf>& evals,
                          vector<float>& fp, vector<unsigned>& ip,
                          const float* ia) {
  if (s.getLength() != 3) return false;

  //Left must always be evaluated
  if (!compile_subexpr(s[1], evals, fp, ip, ia)) return false;

  //Compile the right into separate vectors so we can perform calculate
  //and insert the data we need easily
  vector<evalf> le;
  vector<float> lf;
  vector<unsigned> li;
  if (!compile_subexpr(s[2], le, lf, li, ia)) return false;

  //OK, add our data
  evals.push_back(F);
  ip.push_back(le.size());
  ip.push_back(lf.size());
  ip.push_back(li.size());

  //Append sub data
  evals.insert(evals.end(), le.begin(), le.end());
  ip.insert(ip.end(), li.begin(), li.end());
  fp.insert(fp.end(), lf.begin(), lf.end());

  return true;
}

//Encodes a pseudoinput
template<evalf F>
static bool compile_pseudo(const Setting&, vector<evalf>& evals,
                           vector<float>&, vector<unsigned>&,
                           const float*) {
  evals.push_back(F);
  return true;
}

//Map names to encoding methods
class GeneticAIMapNameToEncodingMethod: public map<string,compiler_function> {
  public:
  GeneticAIMapNameToEncodingMethod() {
    insert("sx",        compile_var<GeneticAI::IOSx>    );
    insert("sy",        compile_var<GeneticAI::IOSy>    );
    insert("st",        compile_var<GeneticAI::IOSt>    );
    insert("svx",       compile_var<GeneticAI::IOSvx>   );
    insert("svy",       compile_var<GeneticAI::IOSvy>   );
    insert("svt",       compile_var<GeneticAI::IOSvt>   );
    insert("tx",        compile_var<GeneticAI::IOTx>    );
    insert("ty",        compile_var<GeneticAI::IOTy>    );
    insert("tt",        compile_var<GeneticAI::IOTt>    );
    insert("tvx",       compile_var<GeneticAI::IOTvx>   );
    insert("tvy",       compile_var<GeneticAI::IOTvy>   );
    insert("tvt",       compile_var<GeneticAI::IOTvt>   );
    insert("tid",       compile_var<GeneticAI::IOTid>   );
    insert("fx",        compile_var<GeneticAI::IOFx>    );
    insert("fy",        compile_var<GeneticAI::IOFy>    );
    insert("ft",        compile_var<GeneticAI::IOFt>    );
    insert("fvx",       compile_var<GeneticAI::IOFvx>   );
    insert("fvy",       compile_var<GeneticAI::IOFvy>   );
    insert("fvt",       compile_var<GeneticAI::IOFvt>   );
    insert("fid",       compile_var<GeneticAI::IOFid>   );
    insert("fieldw",    compile_var<GeneticAI::IOFieldw>        );
    insert("fieldh",    compile_var<GeneticAI::IOFieldh>        );
    insert("time",      compile_var<GeneticAI::IOTime>          );
    insert("rand",      compile_var<GeneticAI::IORand>          );
    insert("noise",     compile_pseudo<v_noise>                 );
    insert("swarmcx",   compile_var<GeneticAI::IOSwarmcx>       );
    insert("swarmcy",   compile_var<GeneticAI::IOSwarmcy>       );
    insert("swarmvx",   compile_var<GeneticAI::IOSwarmvx>       );
    insert("swarmvy",   compile_var<GeneticAI::IOSwarmvy>       );
    insert("paintt",    compile_var<GeneticAI::IOPaintt>        );
    insert("painta",    compile_var<GeneticAI::IOPainta>        );
    insert("painpt",    compile_var<GeneticAI::IOPainpt>        );
    insert("painpa",    compile_var<GeneticAI::IOPainpa>        );
    insert("happys",    compile_var<GeneticAI::IOHappys>        );
    insert("happyg",    compile_var<GeneticAI::IOHappyg>        );
    insert("sads",      compile_var<GeneticAI::IOSads>          );
    insert("sadg",      compile_var<GeneticAI::IOSadg>          );
    insert("nervous",   compile_var<GeneticAI::IONervous>       );
    insert("fear",      compile_var<GeneticAI::IOFear>          );
    insert("frustrate", compile_var<GeneticAI::IOFrustrate>     );
    insert("dislike",   compile_var<GeneticAI::IODislike>       );
    insert("focusmode", compile_var<GeneticAI::IOFocusMode>     );
    insert("focusx",    compile_var<GeneticAI::IOFocusx>        );
    insert("focusy",    compile_var<GeneticAI::IOFocusy>        );
    insert("f2nf",      compile_var<GeneticAI::IOF2nf>          );
    insert("f2ne",      compile_var<GeneticAI::IOF2ne>          );
    insert("ftype",     compile_var<GeneticAI::IOFtype>         );
    insert("f0celld",   compile_var<GeneticAI::IOF0celld>       );
    insert("f0sys0t",   compile_var<GeneticAI::IOF0sys0t>       );
    insert("f0sys0c",   compile_var<GeneticAI::IOF0sys0c>       );
    insert("f0sys1t",   compile_var<GeneticAI::IOF0sys1t>       );
    insert("f0sys1c",   compile_var<GeneticAI::IOF0sys1c>       );
    insert("f0bridge",  compile_var<GeneticAI::IOF0bridge>      );
    insert("smass",     compile_var<GeneticAI::IOSmass>         );
    insert("sacc",      compile_var<GeneticAI::IOSacc>          );
    insert("srota",     compile_var<GeneticAI::IOSrota>         );
    insert("sradius",   compile_var<GeneticAI::IOSradius>       );
    insert("spoweru",   compile_var<GeneticAI::IOSpoweru>       );
    insert("spowerp",   compile_var<GeneticAI::IOSpowerp>       );
    insert("scapac",    compile_var<GeneticAI::IOScapac>        );
    insert("shasweap",  compile_var<GeneticAI::IOShasweap>      );
    insert("swx",       compile_var<GeneticAI::IOSwx>           );
    insert("swy",       compile_var<GeneticAI::IOSwy>           );
    insert("swt",       compile_var<GeneticAI::IOSwt>           );
    insert("throttle",  compile_var<GeneticAI::IOThrottle>      );
    insert("accel",     compile_var<GeneticAI::IOAccel>         );
    insert("brake",     compile_var<GeneticAI::IOBrake>         );
    insert("turning",   compile_var<GeneticAI::IOTurning>       );
    insert("stealth",   compile_var<GeneticAI::IOStealth>       );
    insert("weapon",    compile_var<GeneticAI::IOWeapon>        );
    insert("weaplvl",   compile_var<GeneticAI::IOWeaplvl>       );
    //Work around in error in server-side script
    insert("weaponlvl", compile_var<GeneticAI::IOWeaplvl>       );
    insert("fire",      compile_var<GeneticAI::IOFire>          );
    insert("selfdestr", compile_var<GeneticAI::IOSelfdestr>     );
    insert("mem0",      compile_var<GeneticAI::IOMem0>          );
    insert("mem1",      compile_var<GeneticAI::IOMem1>          );
    insert("mem2",      compile_var<GeneticAI::IOMem2>          );
    insert("mem3",      compile_var<GeneticAI::IOMem3>          );
    insert("mem4",      compile_var<GeneticAI::IOMem4>          );
    insert("mem5",      compile_var<GeneticAI::IOMem5>          );
    insert("mem6",      compile_var<GeneticAI::IOMem6>          );
    insert("mem7",      compile_var<GeneticAI::IOMem7>          );
    insert("tel0",      compile_var<GeneticAI::IOTel0>          );
    insert("tel1",      compile_var<GeneticAI::IOTel1>          );
    insert("tel2",      compile_var<GeneticAI::IOTel2>          );
    insert("tel3",      compile_var<GeneticAI::IOTel3>          );
    insert("tel4",      compile_var<GeneticAI::IOTel4>          );
    insert("tel5",      compile_var<GeneticAI::IOTel5>          );
    insert("tel6",      compile_var<GeneticAI::IOTel6>          );
    insert("tel7",      compile_var<GeneticAI::IOTel7>          );
    insert("+",         compile_fun<f_add,2>                    );
    insert("-",         compile_fun<f_sub,2>                    );
    insert("*",         compile_fun<f_mul,2>                    );
    insert("/",         compile_fun<f_div,2>                    );
    insert("%",         compile_fun<f_mod,2>                    );
    insert("^",         compile_fun<f_exp,2>                    );
    insert("|",         compile_fun<f_abs,1>                    );
    insert("_",         compile_fun<f_neg,1>                    );
    insert("~",         compile_fun<f_not,1>                    );
    insert("=",         compile_fun<f_equ,2>                    );
    insert("<",         compile_fun<f_lt ,2>                    );
    insert(">",         compile_fun<f_gt ,2>                    );
    insert("~=",        compile_fun<f_neq,2>                    );
    insert("<=",        compile_fun<f_leq,2>                    );
    insert(">=",        compile_fun<f_geq,2>                    );
    insert("cos",       compile_fun<f_cos,1>                    );
    insert("sin",       compile_fun<f_sin,1>                    );
    insert("tan",       compile_fun<f_tan,1>                    );
    insert("acos",      compile_fun<f_acos,1>                   );
    insert("asin",      compile_fun<f_asin,1>                   );
    insert("atan",      compile_fun<f_atan,1>                   );
    insert("atan2",     compile_fun<f_atan2,2>                  );
    insert("sqrt",      compile_fun<f_sqrt,1>                   );
    insert("ln",        compile_fun<f_ln, 1>                    );
    insert("||",        compile_logic<f_orl>                    );
    insert("&&",        compile_logic<f_anl>                    );
    insert("if",        compile_if                              );
  }

  private:
  void insert(const char* str, compiler_function fun) {
    string s(str);
    map<string,compiler_function>::insert(make_pair(s,fun));
  }
} static encoders;

static bool compile_subexpr(const Setting& s, vector<evalf>& evals,
                            vector<float>& fparms, vector<unsigned>& iparms,
                            const float* inputs) {
  const char* str;
  switch (s.getType()) {
    case Setting::TypeFloat:
      return compile_const(s, evals, fparms, iparms, inputs);
    case Setting::TypeString:
      //Success if this input exists in encoders and it successfully
      //encodes itself
      str = s;
      return encoders.count(str) && encoders[str](s, evals, fparms, iparms, inputs);
    case Setting::TypeList:
      //Success if:
      return s.getLength() > 0 //The list is not empty
          && s[0].getType() == Setting::TypeString //The first item is a function name
          && encoders.count((const char*)s[0]) //The function is known
          && encoders[(const char*)s[0]]
               (s, evals, fparms, iparms, inputs); //And it encodes successfully
    default:
      return false;
  }
}

/*-***************************************************************************
 * BEGIN: CLASS IMPLEMENTATION                                               *
 *****************************************************************************/

//Maps ship insignias to GeneticAI controlling such ships
static multimap<unsigned long,GeneticAI*> genaiByInsignia;
typedef multimap<unsigned long,GeneticAI*>::iterator gbit;

//Returns the mountpoint for the species of the given name.
static string aimount(const char* name) {
  string ret("ai:");
  ret += name;
  return ret;
}

GeneticAI::GeneticAI(Ship* s)
: Controller(s),
  target(s->target),
  timeSinceRetarget(0),
  ticksUntilUpdate(0),
  timeSinceUpdate(0),
  timeUntilExpensiveUpdate(1024 + rand()%1024),
  nextShipID(1),
  failed(false)
{
  lastInsignia = s->insignia;
  genaiByInsignia.insert(make_pair(s->insignia, this));

  try {
  species=rand() % conf["genai"]["species"].getLength();
  generation=conf["genai"]["species"][species];
  //To cover the instances better, don't randomise the
  //instance number, but keep incrementing it as needed
  static unsigned nextInstance = rand();
  instance=nextInstance++ %
      conf[aimount(conf["genai"]["species"][species].getName())]["list"].getLength();

  memset(basicInputs, 0, sizeof(basicInputs));
  memset(linkedFriends, 0, sizeof(linkedFriends));
  memset(telepathicOutputs, 0, sizeof(telepathicOutputs));
  memset(cumulativeDamage, 0, sizeof(cumulativeDamage));
  memset(outputs, 0, sizeof(outputs));

  compileOutput(IORetarget,     "retarget"      );
  compileOutput(IOScantgt,      "scantgt"       );
  compileOutput(IOFocusMode,    "focusmode"     );
  compileOutput(IOFocusx,       "focusx"        );
  compileOutput(IOFocusy,       "focusy"        );
  compileOutput(IOF1concern,    "f1concern"     );
  compileOutput(IOThrottle,     "throttle"      );
  compileOutput(IOAccel,        "accel"         );
  compileOutput(IOBrake,        "brake"         );
  compileOutput(IOTurning,      "turning"       );
  compileOutput(IOStealth,      "stealth"       );
  compileOutput(IOWeapon,       "weapon"        );
  compileOutput(IOWeaplvl,      "weaplvl"       );
  compileOutput(IOFire,         "fire"          );
  compileOutput(IOSelfdestr,    "selfdestr"     );
  compileOutput(IOMem0,         "mem0"          );
  compileOutput(IOMem1,         "mem1"          );
  compileOutput(IOMem2,         "mem2"          );
  compileOutput(IOMem3,         "mem3"          );
  compileOutput(IOMem4,         "mem4"          );
  compileOutput(IOMem5,         "mem5"          );
  compileOutput(IOMem6,         "mem6"          );
  compileOutput(IOMem7,         "mem7"          );
  compileOutput(IOTel0,         "tel0"          );
  compileOutput(IOTel1,         "tel1"          );
  compileOutput(IOTel2,         "tel2"          );
  compileOutput(IOTel3,         "tel3"          );
  compileOutput(IOTel4,         "tel4"          );
  compileOutput(IOTel5,         "tel5"          );
  compileOutput(IOTel6,         "tel6"          );
  compileOutput(IOTel7,         "tel7"          );
  } catch (...) {
    failed = true;
  }
}

GeneticAI::~GeneticAI() {
  for (unsigned i=0; i<lenof(linkedFriends); ++i)
    if (linkedFriends[i])
      linkedFriends[i]->disconnectTepepathy(this);
  //Remove self from multiset (we don't care about the ship's insignia,
  //but whatever we last used)
  #ifndef NDEBUG
  bool found=false;
  #endif
  pair<gbit,gbit> it = genaiByInsignia.equal_range(lastInsignia);
  for (; it.first != it.second; ++it.first)
    if (it.first->second == this) {
      genaiByInsignia.erase(it.first);
      #ifndef NDEBUG
      found=true;
      #endif
      break;
    }
  assert(found);
}

void GeneticAI::otherShipDied(Ship* that) noth {
  if (that == ship) return;
  Alliance al = getAlliance(ship->insignia, that->insignia);
  float dx = that->getX() - ship->getX(),
        dy = that->getY() - ship->getY();
  float distsq = dx*dx + dy*dy;
  if (al == Allies)
    basicInputs[IOSadg] += 1.0f/distsq;
  else if (al == Enemies)
    basicInputs[IOHappyg] += 1.0f/distsq;

  //Remove any shipid that may be bound to that
  map<Ship*,signed>::iterator it = shipids.find(that);
  if (shipids.end() != it) {
    freeShipids.push_back(it->second);
    shipids.erase(it);
  }
}

void GeneticAI::notifyScore(signed amt) noth {
  basicInputs[IOHappys] += amt;
}

void GeneticAI::damage(float amt, float xoff, float yoff) noth {
  float angle = atan2(yoff,xoff);
  //Add to persistent pain
  if (angle < 0) {
    if (angle < -pi/2) {
      if (angle < -3*pi/4)     cumulativeDamage[0] += amt;
      else                     cumulativeDamage[1] += amt;
    } else {
      if (angle < -pi/4)       cumulativeDamage[2] += amt;
      else                     cumulativeDamage[3] += amt;
    }
  } else {
    if (angle < pi/2) {
      if (angle < pi/4)        cumulativeDamage[4] += amt;
      else                     cumulativeDamage[5] += amt;
    } else {
      if (angle < 3*pi/4)      cumulativeDamage[6] += amt;
      else                     cumulativeDamage[7] += amt;
    }
  }

  //Replace temporary pain if greater
  if (amt > basicInputs[IOPainta]) {
    basicInputs[IOPainta] = amt;
    basicInputs[IOPaintt] = angle;
  }
}

void GeneticAI::disconnectTepepathy(GeneticAI* other) noth {
  //Update emotions
  basicInputs[IOSads] += 1;

  //Remove
  for (unsigned i=0; i<lenof(linkedFriends); ++i) {
    if (linkedFriends[i] == other) {
      linkedFriends[i] = NULL;
      telepathicOutputs[i] = NULL;
    }
  }
}

float* GeneticAI::acceptTelepathy(GeneticAI* other, float* out) noth {
  if (other == this) return NULL;
  signed freeix = -1;
  //Search for free slot/already linked
  for (unsigned i=0; i<lenof(linkedFriends); ++i) {
    if (linkedFriends[i] == other) return NULL;
    if (!linkedFriends[i]) freeix = i;
  }

  if (freeix == -1) return NULL; //No slots free

  //OK
  linkedFriends[freeix] = other;
  telepathicOutputs[freeix] = out;
  //All telepathic inputs are contiguous, so pointer arithmetic
  //will work for this
  return basicInputs+((unsigned)IOTel0)+freeix;
}

//Sums the cost of a particular output.
//Returns the cost, or > MAX_STACK_SIZE if any elements were invalid.
static unsigned expressionComplexity(const Setting& s) noth {
  unsigned cost=0;
  switch (s.getType()) {
    case Setting::TypeFloat:
    case Setting::TypeString:
      return 1;
    case Setting::TypeList:
      for (unsigned i=1; i<s.getLength(); ++i)
        cost += expressionComplexity(s[i]);
      return cost;
    default:
      return MAX_STACK_SIZE+1;
  }
}

static void prettyprint(const Setting& s) {
  switch (s.getType()) {
    case Setting::TypeList:
      cout << "(" << (const char*)s[0];
      for (unsigned i=1; i<s.getLength(); ++i) {
        cout << ' ';
        prettyprint(s[i]);
      }
      cout << ')';
      break;
    case Setting::TypeFloat:
      cout << (float)s;
      break;
    case Setting::TypeString:
      cout << (const char*)s;
      break;
    default:
      break;
  }
}

void GeneticAI::compileOutput(InputOutput output, const char* name) noth {
  Setting& root(conf[aimount(conf["genai"]["species"][species].getName())]["list"][instance]);
  cout << name << ": ";
  prettyprint(root[name]);

  //Save the current sizes
  unsigned esize = allEvalfs.size(),
           isize = allIParms.size(),
           fsize = allFParms.size();
  //Ensure valid and try to compile
  if (root.exists(name)
  &&  expressionComplexity(root[name]) <= MAX_STACK_SIZE
  &&  compile_subexpr(root[name], allEvalfs, allFParms, allIParms, basicInputs)) {
    //Success
    outputs[output].eval = esize;
    outputs[output].fparm = fsize;
    outputs[output].iparm = isize;
    outputs[output].length = allEvalfs.size() - esize;

    cout << ": begin=" << esize << ", fbegin=" << fsize << ", ibegin=" << isize
         << ", length=" << allEvalfs.size()-esize << endl;
  } else {
    //Failure
    //Print warning
    cerr << "WARN: Invalid AI: " << species << ',' << instance << ": " << name << endl;
    //First, truncate the vectors back to what they were
    allEvalfs.resize(esize);
    allFParms.resize(fsize);
    allIParms.resize(isize);

    //Constant zero
    allEvalfs.push_back(v_const);
    allFParms.push_back(0.0f);

    outputs[output].eval = esize;
    outputs[output].fparm = fsize;
    outputs[output].iparm = isize;
    outputs[output].length = 1;
  }
}

signed GeneticAI::getShipID(Ship* that) noth {
  if (that == ship) return -1;
  if (!that->hasPower()) return 0;

  map<Ship*,signed>::const_iterator it = shipids.find(that);
  if (it != shipids.end())
    return it->second;
  else {
    signed s;
    if (freeShipids.empty())
      s = nextShipID++;
    else {
      s = freeShipids.back();
      freeShipids.pop_back();
    }
    shipids[that] = s;
    return s;
  }
}

//Function to pass to lower_bound, using GameField's ordering
//on x (right bound)
static bool compareObjects(const GameObject* a, const GameObject* b) {
  return a->getX()+a->getRadius() < b->getX() + b->getRadius();
}

//Function to pass to sort to sort ships by ascending distance.
//The coordinates must be specified in the
//compareObjectsByDistance_? globals.
static float compareObjectsByDistance_x,
             compareObjectsByDistance_y;
static bool compareObjectsByDistance(const GameObject* a, const GameObject* b) {
  float dxa = a->getX()-compareObjectsByDistance_x,
        dxb = b->getX()-compareObjectsByDistance_x,
        dya = a->getY()-compareObjectsByDistance_y,
        dyb = b->getY()-compareObjectsByDistance_y;
  return dxa*dxa+dya*dya < dxb*dxb + dyb*dyb;
}

float GeneticAI::eval(InputOutput out) noth {
  #if defined(DEBUG) && !defined(WIN32)
  assert(-1 != fedisableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID));
  #endif

  static float stack[MAX_STACK_SIZE];
  float* stackptr = stack;
  entry_point& e(outputs[out]);
  //Reduce the stackptr by one since it is incremented before setting a value
  --stackptr;
  const evalf* begin = &allEvalfs[e.eval];
  const float* fbegin = &allFParms[e.fparm];
  const unsigned* ibegin = &allIParms[e.iparm];
  runEvalfs(begin, begin+e.length, stackptr,
            (unsigned&)ticksUntilUpdate, fbegin, ibegin);

  //The stack should ALWAYS have EXACTLY one element at this point
  assert(stackptr == stack);
  #if defined(DEBUG) && !defined(WIN32)
  assert(-1 != feenableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID));
  #endif

  //OK
  return stack[0];
}

void GeneticAI::update(float et) noth {
  //Handle change of insignia
  if (ship->insignia != lastInsignia) {
    //Remove
    pair<gbit,gbit> it = genaiByInsignia.equal_range(lastInsignia);
    #ifndef NDEBUG
    bool found = false;
    #endif
    for (; it.first != it.second; ++it.first) {
      if (this == it.first->second) {
        genaiByInsignia.erase(it.first);
        #ifndef NDEBUG
        found = true;
        #endif
        break;
      }
    }
    assert(found);

    //Reinsert
    genaiByInsignia.insert(make_pair(ship->insignia, this));
    lastInsignia = ship->insignia;
  }

  timeSinceUpdate += et;
  ticksUntilUpdate -= (unsigned)(et*TICKS_PER_MS);
  timeSinceRetarget += et;
  if (ticksUntilUpdate <= 0) {
    //Trivial inputs
    basicInputs[IOSx] = ship->getX();
    basicInputs[IOSy] = ship->getY();
    basicInputs[IOSt] = ship->getRotation();
    basicInputs[IOSvx] = ship->getVX();
    basicInputs[IOSvy] = ship->getVY();
    basicInputs[IOSvt] = ship->getVRotation();
    if (target.ref) {
      Ship* tgt = (Ship*)target.ref;
      basicInputs[IOTx] = tgt->getX();
      basicInputs[IOTy] = tgt->getY();
      basicInputs[IOTt] = tgt->getRotation();
      basicInputs[IOTvx] = tgt->getVX();
      basicInputs[IOTvy] = tgt->getVY();
      basicInputs[IOTvt] = tgt->getVRotation();
      basicInputs[IOTid] = getShipID(tgt);
      basicInputs[IODislike] = tgt->score;
      basicInputs[IOFrustrate] = tgt->getDamageFrom(ship->blame)/timeSinceRetarget;
    } else basicInputs[IOTid] = 0;
    basicInputs[IOFieldw] = ship->getField()->width;
    basicInputs[IOFieldh] = ship->getField()->height;
    basicInputs[IOTime] += timeSinceUpdate;
    basicInputs[IORand] = (rand()%RAND_MAX)/(float)RAND_MAX;
    //Accumulate values for swarm inputs, then average
    unsigned swarmc = 1;
    basicInputs[IOSwarmcx] = ship->getX();
    basicInputs[IOSwarmcy] = ship->getY();
    basicInputs[IOSwarmvx] = ship->getVX();
    basicInputs[IOSwarmvy] = ship->getVY();
    for (unsigned i=0; i<numTelepathy; ++i) {
      if (linkedFriends[i]) {
        ++swarmc;
        Ship* s = linkedFriends[i]->ship;
        basicInputs[IOSwarmcx] += s->getX();
        basicInputs[IOSwarmcy] += s->getY();
        basicInputs[IOSwarmvx] += s->getVX();
        basicInputs[IOSwarmvy] += s->getVY();
      }
    }
    basicInputs[IOSwarmcx] /= swarmc;
    basicInputs[IOSwarmcy] /= swarmc;
    basicInputs[IOSwarmvx] /= swarmc;
    basicInputs[IOSwarmvy] /= swarmc;

    basicInputs[IOPainta] *= pow(0.5f, timeSinceUpdate/2000.0f);
    float m10 = pow(0.5f, timeSinceUpdate/10000.0f);
    basicInputs[IOHappys] *= m10;
    basicInputs[IOHappyg] *= m10;
    basicInputs[IOSads] *= m10;
    basicInputs[IOSadg] *= m10;
    float* maxpainp = max_element(cumulativeDamage, cumulativeDamage+lenof(cumulativeDamage));
    basicInputs[IOPainpt] = (maxpainp-cumulativeDamage)*2*pi/8.0f - pi;
    basicInputs[IOPainpa] = *maxpainp;

    basicInputs[IOSmass] = ship->getMass();
    basicInputs[IOSacc] = ship->getAcceleration();
    basicInputs[IOSrota] = ship->getRotationAccel();
    basicInputs[IOSradius] = ship->getRadius();
    basicInputs[IOSpoweru] = ship->getPowerDrain();
    basicInputs[IOSpowerp] = ship->getPowerSupply();
    basicInputs[IOScapac] = ship->getCurrentCapacitance();

    /* Parts of the function below assume that the field is in X-sorted
     * order. This is not entirely the case, as this executes during the
     * update of the field. However, the field will still be in nearly
     * sorted order, so any anomalies will be minimal.
     */

    //Nontrivial inputs
    timeUntilExpensiveUpdate -= timeSinceUpdate;
    if (timeUntilExpensiveUpdate <= 0) {
      timeUntilExpensiveUpdate = rand()%1024 + 1024;
      basicInputs[IONervous] = 0;
      basicInputs[IOFear] = 0;

      GameField::iterator begin = ship->getField()->begin(),
                          end   = ship->getField()->end();
      GameField::iterator base =
          lower_bound(begin, end, ship, compareObjects);
      //Look at all objects within 3 screens (rectangle)
      float minx = ship->getX()-3, maxx = ship->getX()+3,
            miny = ship->getY()-3, maxy = ship->getY()+3;
      //Move base to minx
      if (base == end && base != begin) --base;
      while (base != begin && (*base)->getX() >= minx) --base;
      //Sum the parts for nervous and fear.
      //  nervous: for enemy ships, add mass*adot/distanceSquared
      //    where adot is dx*cos(theta)+dy*sin(theta)
      //  fear: for weapons, add threat*vdot/distanceSquared
      //    where threat is 1 for LightWeapon and 2 for HeavyWeapon,
      //    and vdot is dx*vx + dy*vy
      for (GameField::iterator it=base; it != end && (*it)->getX() < maxx; ++it) {
        if ((*it)->getY() >= miny && (*it)->getY() < maxy) {
          if ((*it)->getClassification() == GameObject::ClassShip) {
            Ship* s = (Ship*)*it;
            if (s->hasPower() && Enemies == getAlliance(s->insignia,ship->insignia)) {
              float dx = ship->getX() - s->getX();
              float dy = ship->getY() - s->getY();
              float theta = s->getRotation();
              basicInputs[IONervous] +=
                  max(0.0f, s->getMass()*(dx*cos(theta)+dy*sin(theta))/(dx*dx+dy*dy));
            }
          } else if ((*it)->getClassification() == GameObject::LightWeapon
                 ||  (*it)->getClassification() == GameObject::HeavyWeapon) {
            GameObject* go = *it;
            float dx = ship->getX() - go->getX();
            float dy = ship->getY() - go->getY();
            float threat = (go->getClassification() == GameObject::LightWeapon?
                            1 : 2);
            basicInputs[IOFear] +=
                max(0.0f, threat*(dx*go->getVX() + dy*go->getVY())/(dx*dx+dy*dy));
          }
        }
      }

      //If we aren't full, search for friends
      {
        pair<gbit,gbit> it = genaiByInsignia.equal_range(ship->insignia);
        //Pick one at random
        //(We need to find the length by counting...)
        unsigned length=0;
        for (gbit g = it.first; g != it.second; ++g)
          ++length;
        for (unsigned i=0; i<numTelepathy && i+1 < length; ++i) {
          if (!linkedFriends[i]) {
            unsigned off = rand()%length;
            gbit g;
            for (g=it.first; off; --off) ++g;
            if (float* out =
                g->second->acceptTelepathy(this, basicInputs+i+(unsigned)IOTel0)) {
              linkedFriends[i] = g->second;
              telepathicOutputs[i] = out;
            }
          }
        }
      }
    }

    //Actual evaluation
    bool retarget = f2b(eval(IORetarget));
    if (retarget) {
      //Enumerate possible targets
      vector<Ship*> possibilities;
      radar_t::iterator begin, end;
      ship->radarBounds(begin,end);
      for (; begin != end; ++begin) {
        if (Enemies == getAlliance(ship->insignia, (*begin)->insignia))
          possibilities.push_back(*begin);
      }

      compareObjectsByDistance_x = ship->getX();
      compareObjectsByDistance_y = ship->getY();
      sort(possibilities.begin(), possibilities.end(), compareObjectsByDistance);

      //See which one the AI wants to target
      for (unsigned i=0; i<possibilities.size(); ++i) {
        //Alter inputs
        Ship* t = possibilities[i];
        basicInputs[IOTx] = t->getX();
        basicInputs[IOTy] = t->getY();
        basicInputs[IOTt] = t->getRotation();
        basicInputs[IOTvx] = t->getVX();
        basicInputs[IOTvy] = t->getVY();
        basicInputs[IOTvt] = t->getVRotation();
        basicInputs[IOTid] = getShipID(t);
        if (f2b(eval(IOScantgt))) {
          target.assign(possibilities[i]);
          timeSinceRetarget = 0;
          break;
        }
      }
    } else {
      //Phase 1: select focus mode
      basicInputs[IOFocusMode] = eval(IOFocusMode);
      unsigned focusMode = f2u(basicInputs[IOFocusMode],3);
      //Phase 2: select focus location
      float focusx = eval(IOFocusx);
      float focusy = eval(IOFocusy);
      basicInputs[IOFocusx] = focusx;
      basicInputs[IOFocusy] = focusy;
      focusx += ship->getX();
      focusy += ship->getY();

      switch (focusMode) {
        case 0: {
          //Create a dummy object to use for finding collisions
          Blast dummy(ship->getField(), 0, focusx, focusy, STD_CELL_SZ/8, 0);
          Ship* what = NULL;
          //Check self, target, then craft on radar
          if (objectsCollide(&dummy,ship))
            what = ship;
          else if (target.ref && objectsCollide(&dummy,target.ref))
            what = (Ship*)target.ref;
          else {
            //Scan through radar
            radar_t::iterator it, end;
            ship->radarBounds(it,end);
            for (; it != end; ++it) {
              if (objectsCollide(&dummy,*it)) {
                what = *it;
                break;
              }
            }
          }

          if (what) {
            //Fill focus information out
            basicInputs[IOFx] = what->getX();
            basicInputs[IOFy] = what->getY();
            basicInputs[IOFt] = what->getRotation();
            basicInputs[IOFvx] = what->getVX();
            basicInputs[IOFvy] = what->getVY();
            basicInputs[IOFvt] = what->getVRotation();
            basicInputs[IOFid] = getShipID(what);
            Alliance al = getAlliance(what->insignia,ship->insignia);
            if (what == ship)
              basicInputs[IOFtype] = -1;
            else if (al == Allies)
              basicInputs[IOFtype] = 2;
            else if (al == Neutral)
              basicInputs[IOFtype] = 1;
            else
              basicInputs[IOFtype] = 3;

            //Find the Cell* the Blast collides with
            CollisionRectangle& dcr(*(*dummy.getCollisionBounds())[0]);
            Cell* collided = NULL;
            for (unsigned i=0; i<what->cells.size(); ++i) {
              if (what->cells[i]->getCollisionBounds() &&
                  rectanglesCollide(dcr,*what->cells[i]->getCollisionBounds())) {
                collided = what->cells[i];
                break;
              }
            }

            if (collided) {
              basicInputs[IOF0bridge] = b2f(collided->usage == CellBridge);
              basicInputs[IOF0celld] =
                  1.0f - collided->getCurrDamage() / collided->getMaxDamage();
              for (unsigned s=0; s<2; ++s) {
                float c=0,t=0;
                if (collided->systems[s]) {
                  const type_info& ti(typeid(*collided->systems[s]));
                  if (false); //So the macro below is less verbose
                  #define S(clazz,cv,tv) \
                    else if (ti == typeid(clazz)) c=cv, t=tv
                  S(Capacitor,                  1,2);
                  S(EnergyChargeLauncher,       1,3);
                  S(FissionPower,               1,1);
                  S(MagnetoBombLauncher,        1,3);
                  S(MiniGravwaveDrive,          1,5);
                  S(ParticleAccelerator,        1,5);
                  S(PowerCell,                  1,1);
                  S(ReinforcementBulkhead,      1,4);
                  S(SelfDestructCharge,         1,6);
                  S(BussardRamjet,              2,5);
                  S(FusionPower,                2,1);
                  S(Heatsink,                   2,6);
                  S(MiniGravwaveDriveMKII,      2,5);
                  S(PlasmaBurstLauncher,        2,3);
                  S(SemiguidedBombLauncher,     2,3);
                  S(ShieldGenerator,            2,4);
                  S(SuperParticleAccelerator,   2,5);
                  S(AntimatterPower,            3,1);
                  S(CloakingDevice,             3,6);
                  S(DispersionShield,           3,4);
                  S(GatlingPlasmaBurstLauncher, 3,3);
                  S(MissileLauncher,            3,3);
                  S(MonophasicEnergyEmitter,    3,3);
                  S(ParticleBeamLauncher,       3,3);
                  S(RelIonAccelerator,          3,5);
                  #undef S
                }
                basicInputs[s? IOF0sys1c : IOF0sys0c] = c;
                basicInputs[s? IOF0sys1t : IOF0sys0t] = t;
              }
            } else {
              //No cell, same as destroyed
              basicInputs[IOF0celld] = 0;
              basicInputs[IOF0sys0c] = 0;
              basicInputs[IOF0sys1c] = 0;
              basicInputs[IOF0sys0t] = 0;
              basicInputs[IOF0sys1t] = 0;
              basicInputs[IOF0bridge] = b2f(false);
            }
          } else {
            //Null info
            basicInputs[IOFtype] = 0;
            basicInputs[IOFid] = 0;
            basicInputs[IOF0bridge] = b2f(false);
            basicInputs[IOF0sys0t] = 0;
            basicInputs[IOF0sys1t] = 0;
            basicInputs[IOF0sys0c] = 0;
            basicInputs[IOF0sys1c] = 0;
          }
        } break;
        case 1: {
          //Scan objects around the area, evaluating concern;
          //set f* according to that object with maximum concern
          float minx = focusx-1, maxx = focusx+1;
          float miny = focusy-1, maxy = focusy+1;

          GameObject* focussed = NULL;
          float maxConcern = 0; //Initialise to suppress compiler warning
          GameField::iterator base, begin, end;
          begin = ship->getField()->begin();
          end = ship->getField()->end();
          //Create dummy object for searching
          Blast dummy(ship->getField(), 0, minx, focusy, 0.1f, 0);
          base = lower_bound(begin, end, &dummy, compareObjects);
          if (base == end && base != begin) --base;

          //Base is at minimum X, scan the objects
          for (GameField::iterator it=base; it != end && (*it)->getX() < maxx; ++it) {
            GameObject* go = *it;
            if (go->getY() > miny && go->getY() < maxy) {
              //Set parms
              switch (go->getClassification()) {
                case GameObject::ClassShip: {
                  Ship* s = (Ship*)go;
                  Alliance al = getAlliance(ship->insignia, s->insignia);
                  if      (s == ship)
                    basicInputs[IOFtype] = -1;
                  else if (!s->hasPower() || al == Neutral)
                    basicInputs[IOFtype] = 1;
                  else if (al == Allies)
                    basicInputs[IOFtype] = 2;
                  else
                    basicInputs[IOFtype] = 3;
                  basicInputs[IOFid] = getShipID(s);
                } break;
                case GameObject::LightWeapon: {
                  basicInputs[IOFtype] = 4;
                } break;
                case GameObject::HeavyWeapon: {
                  basicInputs[IOFtype] = 5;
                } break;
                default: continue; //Don't care about this type of object
              }
              basicInputs[IOFx] = go->getX();
              basicInputs[IOFy] = go->getY();
              basicInputs[IOFvx] = go->getVX();
              basicInputs[IOFvy] = go->getVY();

              float concern = eval(IOF1concern);
              if (!focussed || concern > maxConcern) {
                focussed = go;
                maxConcern = concern;
              }
            }
          }

          //Set parms for later phases
          if (!focussed) {
            basicInputs[IOFtype] = 0;
          } else {
            GameObject* go = focussed;
            basicInputs[IOFx] = go->getX();
            basicInputs[IOFy] = go->getY();
            basicInputs[IOFvx] = go->getVX();
            basicInputs[IOFvy] = go->getVY();
            switch (go->getClassification()) {
              case GameObject::ClassShip: {
                Ship* s = (Ship*)go;
                Alliance al = getAlliance(ship->insignia, s->insignia);
                if      (s == ship)
                  basicInputs[IOFtype] = -1;
                else if (!s->hasPower() || al == Neutral)
                  basicInputs[IOFtype] = 1;
                else if (al == Allies)
                  basicInputs[IOFtype] = 2;
                else
                  basicInputs[IOFtype] = 3;
                basicInputs[IOFid] = getShipID(s);
              } break;
              case GameObject::LightWeapon: {
                basicInputs[IOFtype] = 4;
              } break;
              case GameObject::HeavyWeapon: {
                basicInputs[IOFtype] = 5;
              } break;
              default: break; //Suppress compiler warning
            }
          }
        } break;
        case 2: {
          //Count enemies and friends in focussed area
          basicInputs[IOF2nf] = 0;
          basicInputs[IOF2ne] = 0;
          radar_t::iterator begin, end;
          ship->radarBounds(begin,end);
          for (; begin != end; ++begin) {
            Ship* s = *begin;
            if (s->getX() > ship->getX()-5 && s->getX() < ship->getX()+5
            &&  s->getY() > ship->getY()-5 && s->getY() < ship->getY()+5) {
              if (Allies == getAlliance(ship->insignia, s->insignia))
                basicInputs[IOF2nf] += 1;
              else if (Enemies == getAlliance(ship->insignia, s->insignia))
                basicInputs[IOF2ne] += 1;
            }
          }
        } break;
      }

      //Done with focus, move on to weapon selection
      //Phase 7
      basicInputs[IOWeapon] = eval(IOWeapon);
      Weapon weap = (Weapon)f2u(basicInputs[IOWeapon], 8);

      //Get weapon information
      vector<ShipSystem*> weapons;
      weapon_enumerate(ship, weap, weapons);
      if (weapons.empty()) {
        basicInputs[IOShasweap] = b2f(false);
      } else {
        basicInputs[IOShasweap] = b2f(true);
        //Select random launcher
        Launcher* launcher = (Launcher*)weapons[rand()%weapons.size()];
        basicInputs[IOSwx] = launcher->container->getX();
        basicInputs[IOSwy] = launcher->container->getY();
        basicInputs[IOSwt] = launcher->getLaunchAngle();
      }

      //Phase 8, level selection
      //(This doesn't wrap, but is clamped by weapon_setEnergyLevel)
      basicInputs[IOWeaplvl] = eval(IOWeaplvl);
      signed lvl = (signed)max(0.0f,min(basicInputs[IOWeaplvl],1000.0f));

      //Phase 9, other control, memory, telepathy
      float f9throttle = eval(IOThrottle);
      float f9accel = eval(IOAccel);
      float f9brake = eval(IOBrake);
      float f9turning = eval(IOTurning);
      float f9stealth = eval(IOStealth);
      float f9fire = eval(IOFire);
      float f9selfdestr = eval(IOSelfdestr);

      //Memory is contiguous
      float memout[8];
      for (unsigned i=(unsigned)IOMem0; i <= (unsigned)IOMem7; ++i)
        memout[i-(unsigned)IOMem0] = eval((InputOutput)i);

      //Telepathy is also contiguous
      for (unsigned i=0; i<numTelepathy; ++i)
        if (linkedFriends[i])
          *telepathicOutputs[i] = eval((InputOutput)(i+(unsigned)IOTel0));

      //Copy back into basicInputs
      basicInputs[IOThrottle] = f9throttle;
      basicInputs[IOAccel] = f9accel;
      basicInputs[IOBrake] = f9brake;
      basicInputs[IOTurning] = f9turning;
      basicInputs[IOStealth] = f9stealth;
      basicInputs[IOFire] = f9fire;
      basicInputs[IOSelfdestr] = f9selfdestr;
      for (unsigned i=(unsigned)IOMem0; i <= (unsigned)IOMem7; ++i)
        basicInputs[i] = memout[i-(unsigned)IOMem0];

      //Handle non-turning output
      //(Turning is handled every frame separately)
      ship->configureEngines(f2b(basicInputs[IOAccel]), f2b(basicInputs[IOBrake]),
                             basicInputs[IOThrottle]);
      ship->setStealthMode(f2b(basicInputs[IOStealth]));
      if (f2b(basicInputs[IOFire])) {
        weapon_setEnergyLevel(ship, weap, lvl);
        weapon_fire(ship, weap);
      }
      if (f2b(basicInputs[IOSelfdestr])) {
        selfDestruct(ship);
      }
    }

    timeSinceUpdate = 0;
  }

  ship->spin(max(-1.0f, min(+1.0f, basicInputs[IOTurning]))*STD_ROT_RATE*et);
}

static void calculateGeneticAIFunctionCostsHelper(const char* name,
                                                  evalf f) {
  float stack[3] = {1,1,1};
  float fparm[1] = {0};
  unsigned iparm[] = {0,0,0,0,0,0,0,0};
  clock_t begin = clock();
  unsigned cost=0;
  for (unsigned i=0; i<100000000; ++i) {
    float* sp = stack;
    const float* fp = fparm;
    const unsigned* ip = iparm;
    const void*const* n = NULL;
    f(sp, fp, ip, cost, n);
  }
  clock_t end = clock();

  cout << name << '\t' << (end-begin)/10000 << endl;
}

void calculateGeneticAIFunctionCosts() {
  #define F calculateGeneticAIFunctionCostsHelper
  F("add", f_add);
  F("sub", f_sub);
  F("mul", f_mul);
  F("div", f_div);
  F("mod", f_mod);
  F("exp", f_exp);
  F("abs", f_abs);
  F("neg", f_neg);
  F("not", f_not);
  F("equ", f_equ);
  F("lt" , f_lt );
  F("gt" , f_gt );
  F("neq", f_neq);
  F("geq", f_geq);
  F("leq", f_leq);
  F("cos", f_cos);
  F("sin", f_sin);
  F("tan", f_tan);
  F("acos", f_acos);
  F("asin", f_asin);
  F("atan", f_atan);
  F("atan2", f_atan2);
  F("sqrt", f_sqrt);
  F("ln" , f_ln );
  #undef F
}
