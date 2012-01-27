/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/genai/cortex.hxx
 */

/*
 * cortex.cxx
 *
 *  Created on: 27.10.2011
 *      Author: jason
 */

#include <map>
#include <string>
#include <set>
#include <vector>
#include <utility>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <cassert>
#include <cfloat>
#if !defined(WIN32) && defined(DEBUG)
#include </usr/include/fenv.h>
#endif

#include <libconfig.h++>

#include "cortex.hxx"
#include "src/globals.hxx"

using namespace std;
using namespace libconfig;

#define EXTREE_F_MINIX 0 //inclusive minimum index for functions
#define EXTREE_F_MAXIX 31 //2^5-1, exclusive maximum index for functions
#define EXTREE_I_MINIX EXTREE_F_MAXIX //inclusive minimum index for inputs/consts
#define EXTREE_I_MAXIX 63 //2^6-1, exclusive maximum index for inputs
#define EXTREE_LEN EXTREE_I_MAXIX //Length of expression tree list
#define EXTREE_LEVELS 6 //Number of levels (including constant level)

//Implement template class functions so they can be used below
//(For organisation's sake, also put non-template compiler and
// evaluator functions here)
template<Cortex::evaluator E, unsigned Arity>
void Cortex::c_fun(vector<execdat>& dst, const Setting& list, unsigned ix) {
  if (Arity >= 1) compile(dst, list, ix*2+1);
  if (Arity == 2) compile(dst, list, ix*2+2);
  execdat dat;
  dat.eval = E;
  dst.push_back(dat);
}
void Cortex::c_const(vector<execdat>& dst, const Setting& list, unsigned ix) {
  execdat dat;
  dat.eval = &Cortex::f_const;
  dst.push_back(dat);
  dat.f = (float)list[ix];
  dst.push_back(dat);
}
void Cortex::c_input(vector<execdat>& dst, const Setting& list, unsigned ix) {
  map<string,unsigned>::const_iterator it = inputMap.find(string((const char*)list[ix]));
  if (it == inputMap.end()) {
    cerr << "WARN: Unknown input in cortex " << cortexType
         << ", instance " << instance << ": " << (const char*)list[ix] << endl;
    //Encode zero
    execdat dat;
    dat.eval = &Cortex::f_const;
    dst.push_back(dat);
    dat.f = 0;
    dst.push_back(dat);
    return;
  }

  //OK, encode the input
  execdat dat;
  dat.eval = &Cortex::f_input;
  dst.push_back(dat);
  dat.in = &inputs[it->second];
  dst.push_back(dat);
}
void Cortex::c_nop(vector<execdat>& dst, const Setting& list, unsigned ix) {
  compile(dst, list, ix*2+1);
}

class CortexFunctionMap: public map<string,Cortex::compiler> {
  public:
  CortexFunctionMap() {
    ins("+",    &Cortex::c_fun<&Cortex::f_add,2>);
    ins("-",    &Cortex::c_fun<&Cortex::f_sub,2>);
    ins("*",    &Cortex::c_fun<&Cortex::f_mul,2>);
    ins("/",    &Cortex::c_fun<&Cortex::f_div,2>);
    ins("%",    &Cortex::c_fun<&Cortex::f_mod,2>);
    ins("^",    &Cortex::c_fun<&Cortex::f_pow,2>);
    ins("_",    &Cortex::c_fun<&Cortex::f_neg,1>);
    ins("|",    &Cortex::c_fun<&Cortex::f_abs,1>);
    ins("~",    &Cortex::c_fun<&Cortex::f_anm,1>);
    ins("=",    &Cortex::c_nop);
    ins("sqrt", &Cortex::c_fun<&Cortex::f_sqr,1>);
    ins("rand", &Cortex::c_fun<&Cortex::f_rnd,0>);
    ins("max",  &Cortex::c_fun<&Cortex::f_max,2>);
    ins("min",  &Cortex::c_fun<&Cortex::f_min,2>);
    ins("cos",  &Cortex::c_fun<&Cortex::f_cos,1>);
    ins("sin",  &Cortex::c_fun<&Cortex::f_sin,1>);
    ins("tan",  &Cortex::c_fun<&Cortex::f_tan,1>);
    ins("acos", &Cortex::c_fun<&Cortex::f_aco,1>);
    ins("asin", &Cortex::c_fun<&Cortex::f_asi,1>);
    ins("atan", &Cortex::c_fun<&Cortex::f_ata,1>);
    ins("atan2",&Cortex::c_fun<&Cortex::f_at2,2>);
  }

  private:
  void ins(const char* str, Cortex::compiler f) {
    insert(make_pair(string(str), f));
  }
} static cortexFunctionMap;
void Cortex::compile(vector<execdat>& dst, const Setting& list, unsigned ix) {
  if (ix < EXTREE_F_MAXIX) {
    //Function, make sure string
    if (list[ix].getType() != Setting::TypeString) {
      //Nope, push zero
      cerr << "WARN: Cortex " << cortexType << ", instance " << instance
           << ": non-string in function location, index " << ix << endl;
      execdat dat;
      dat.eval = &Cortex::f_const;
      dst.push_back(dat);
      dat.f = 0;
      dst.push_back(dat);
      return;
    }
    CortexFunctionMap::const_iterator it =
        cortexFunctionMap.find(string((const char*)list[ix]));
    if (it == cortexFunctionMap.end()) {
      //Function not found
      cerr << "WARN: Cortex " << cortexType << ", instance " << instance
           << ": unknown function " << (const char*)list[ix] << endl;
      execdat dat;
      dat.eval = &Cortex::f_const;
      dst.push_back(dat);
      dat.f = 0;
      dst.push_back(dat);
      return;
    }
    //OK
    (this->*(it->second))(dst, list, ix);
  } else {
    //Constant or input
    if (Setting::TypeFloat == list[ix].getType())
      c_const(dst, list, ix);
    else if (Setting::TypeString == list[ix].getType())
      c_input(dst, list, ix);
    else {
      //Unexpected type
      cerr << "WARN: Cortex " << cortexType << ", instance " << instance
           << ": unexpected type in constant location, index " << ix << endl;
      execdat dat;
      dat.eval = &Cortex::f_const;
      dst.push_back(dat);
      dat.f = 0;
      dst.push_back(dat);
      return;
    }
  }
}

void Cortex::f_add(float*& stack, const execdat*&) {
  float a = *stack--;
  *stack += a;
}
void Cortex::f_sub(float*& stack, const execdat*&) {
  float a = *stack--;
  *stack -= a;
}
void Cortex::f_mul(float*& stack, const execdat*&) {
  float a = *stack--;
  *stack *= a;
}
void Cortex::f_div(float*& stack, const execdat*&) {
  float den = *stack--;
  if (fabs(den) < 1.0e-20f) *stack = 1;
  else *stack /= den;
}
void Cortex::f_mod(float*& stack, const execdat*&) {
  float base = *stack--;
  if (fabs(base) < 1.0e-20f) *stack = 1;
  else {
    *stack = fmod(*stack, base);
    if (*stack < -0.0f) *stack += base;
  }
}
void Cortex::f_pow(float*& stack, const execdat*&) {
  float power = *stack--, base = *stack;
  if (base <= 0.0f) *stack = 1.0f;
  else *stack = pow(power,base);
}
void Cortex::f_neg(float*& stack, const execdat*&) {
  *stack = -*stack;
}
void Cortex::f_abs(float*& stack, const execdat*&) {
  *stack = fabs(*stack);
}
void Cortex::f_anm(float*& stack, const execdat*&) {
  *stack = fmod(*stack, 2*pi); //This results in [-2π..+2π]
  //Convert to [-π..+π]
  if (*stack < -pi) *stack += 2*pi;
  else if (*stack > +pi) *stack -= 2*pi;
}
void Cortex::f_sqr(float*& stack, const execdat*&) {
  if (*stack >= +0.0f) *stack = sqrt(*stack);
  else                *stack = 0.0f;
}
void Cortex::f_rnd(float*& stack, const execdat*&) {
  *++stack = rand()%RAND_MAX / (float)RAND_MAX;
}
void Cortex::f_max(float*& stack, const execdat*&) {
  float a = *stack--;
  *stack = max(*stack,a);
}
void Cortex::f_min(float*& stack, const execdat*&) {
  float a = *stack--;
  *stack = min(*stack,a);
}
void Cortex::f_cos(float*& stack, const execdat*&) {
  *stack = cos(*stack);
}
void Cortex::f_sin(float*& stack, const execdat*&) {
  *stack = sin(*stack);
}
void Cortex::f_tan(float*& stack, const execdat*&) {
  *stack = tan(*stack);
}
void Cortex::f_aco(float*& stack, const execdat*&) {
  if (fabs(*stack) <= 1.0f)
    *stack = acos(*stack);
  else
    *stack = 0;
}
void Cortex::f_asi(float*& stack, const execdat*&) {
  if (fabs(*stack) <= 1.0f)
    *stack = asin(*stack);
  else
    *stack = 0;
}
void Cortex::f_ata(float*& stack, const execdat*&) {
  *stack = atan(*stack);
}
void Cortex::f_at2(float*& stack, const execdat*&) {
  float x = *stack--, y=*stack;
  if (fabs(x) == 0.0f && fabs(y) == 0.0f)
    *stack = 0;
  else
    *stack = atan2(y,x);
}

void Cortex::f_input(float*& stack, const execdat*& exe) {
  *++stack = *exe++->in;
}
void Cortex::f_const(float*& stack, const execdat*& exe) {
  *++stack = exe++->f;
}

Cortex::Cortex(const Setting& species, const char* cname, unsigned nin, unsigned nout,
               const map<string,unsigned>& inmap)
: outputs(nout), inputMap(inmap), cortexType(cname),
  baseConfigValid(species.exists(cname) && species[cname].getType() == Setting::TypeList
              &&  species[cname].getLength() > 0),
  instance(baseConfigValid? rand()%species[cname].getLength() : 0),
  configAcceptable(baseConfigValid && species[cname][instance].getType() == Setting::TypeGroup),
  root(configAcceptable? species[cname][instance] : *(const Setting*)0),
  inputs(nin)
{
}

void Cortex::compileOutput(const char* name, unsigned index) throw() {
  output& out = outputs[index];
  unsigned startix = evaluators.size();
  if (root.exists(name) && root[name].getType() == Setting::TypeList
  &&  EXTREE_LEN == root[name].getLength())
    compile(evaluators, root[name], 0);
  else {
    //Missing output
    cerr << "WARN: Cortex " << cortexType << ", instance " << instance
         << ": missing output: " << name << endl;
    //Use constant zero
    execdat dat;
    dat.eval = &Cortex::f_const;
    evaluators.push_back(dat);
    dat.f = 0;
    evaluators.push_back(dat);
  }

  unsigned endix = evaluators.size();
  out.begin = startix;
  out.end = endix;
}

float Cortex::eval(unsigned index) const throw() {
  //Certain functions can cause floating-point exceptions we
  //can't easily prevent, so temporarily disable FPEs while
  //evaluating.
  #if defined(DEBUG) && !defined(WIN32)
  //Not sure why, but if we don't do this the next statement fails
  //with an FPE... (this doesn't happen in any other location  where
  //we temporarily disable FPEs)
  feclearexcept(0xFFFFFFF);
  assert(-1 != fedisableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID));
  #endif

  /* The stack will have a maximum size equal to the number
   * of levels. Worst case:
   *        f0
   *       / \
   *      s0  f1
   *          / \
   *         s1 f2
   *            / \
   *           s2 f3
   *              / \
   *             s3 f4
   *                / \
   *               s4 s5
   * Where f is an unevaluated function and s is a constant or
   * evaluated subtree (note exactly five levels of functions,
   * the sixth leaf level, and that the number of stack elements
   * is the same as levels because the top level does not contribute,
   * while the leaf level contributes two, and all other contribute one).
   */
  float stack[EXTREE_LEVELS];
  float* stackptr = stack-1; //Pushing preincrements the pointer, so start off below the array
  const execdat* exe = &evaluators[outputs[index].begin],
               * end = &evaluators[outputs[index].end];
  while (exe != end) {
    //Must be separated into two statements, since
    //  exe++->eval(stackptr, exe)
    //has no defined result
    const execdat* curr = exe++;
    curr->eval(stackptr, exe);
  }

  assert(stackptr == stack); //If all went well, exactly one item is on the stack

  #if defined(DEBUG) && !defined(WIN32)
  assert(-1 != feenableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID));
  #endif

  //Handle nonfinite cases
  if (stack[0] != stack[0])
    stack[0] = 0;
  else if (stack[0] > FLT_MAX)
    stack[0] = FLT_MAX;
  else if (stack[0] < -FLT_MAX)
    stack[0] = -FLT_MAX;

  return stack[0];
}

float Cortex::parm(const char* name) const throw() {
  if (root.exists(name)
  &&  root[name].getType() == Setting::TypeFloat)
    return root[name];
  else
    return 0.0f;
}
