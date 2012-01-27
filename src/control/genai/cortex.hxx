/**
 * @file
 * @author Jason Lingle
 * @brief Defines the Cortex class, a component of the GenAI.
 */

/*
 * cortex.hxx
 *
 *  Created on: 25.10.2011
 *      Author: jason
 */

#ifndef CORTEX_HXX_
#define CORTEX_HXX_

#include <vector>
#include <map>
#include <string>

namespace libconfig {class Setting;}

/**
 * The Cortex class describes a single processing unit of the genetic AI.
 * This class by itself only defines compilation, execution, and input/output
 * functions; actual logic (and defining what exactly these inputs and outputs
 * are) must be done by subclasses.
 *
 * This class is not to be used directly; ie, you should never have a Cortex*
 * or Cortex& (a Cortex is useless); specifically, no functions are virtual, not
 * even the destructor.
 */
class Cortex {
  friend class CortexFunctionMap;

  public:
  union execdat;
  //The function type for functions to evaluate an expression.
  //They take the stack and a pointer to the next execution datum;
  //some will read an element from this and skip executing it,
  //such as constants.
  typedef void (*evaluator)(float*&, const execdat*&);
  //The types of data stored in the execution vector; each may store
  //an evaluator, a constant value, or an input reference.
  union execdat {
    evaluator eval;
    float f;
    const float* in;
  };
  typedef void (Cortex::*compiler)(std::vector<execdat>& exec,
                                   const libconfig::Setting& funs,
                                   unsigned ix);

  private:
  //Contains information on the entry and exit points of an output
  struct output {
    unsigned begin, end;
  };
  std::vector<output> outputs;
  std::vector<execdat> evaluators;

  //Maps names to inputs
  const std::map<std::string,unsigned>& inputMap;

  public:
  /**
   * The type name of this cortex.
   */
  const char*const cortexType;
  /**
   * True iff the base config was valid.
   */
  const bool baseConfigValid;

  /**
   * The instance of this cortex within its cortex type.
   */
  const unsigned instance;

  /**
   * True iff the config is valid enough for the Cortex to function.
   */
  const bool configAcceptable;

  /**
   * The root setting for the species.
   * This is only defined if configAcceptable is true!
   */
  const libconfig::Setting& root;

  protected:
  /**
   * Contains each input's value.
   */
  std::vector<float> inputs;

  /**
   * Constructs a Cortex for the given number of inputs and outputs,
   * and provides a map that maps input names to input indices.
   * @param species the Setting that contains this species
   * @param cortexName the name of the group to look in to select an
   *   instance and to use as a compilation source.
   * @param ninputs number of inputs
   * @param noutputs number of outputs
   * @param inputMap maps input names to input indices.
   *   This will not be accessed in the constructor, so it is OK
   *   to pass an unconstructed object.
   */
  Cortex(const libconfig::Setting& species, const char* cortexName,
         unsigned ninputs, unsigned noutputs,
         const std::map<std::string,unsigned>& inputMap);

  /**
   * Compiles the given output. If it fails for any reason,
   * it is results in a constant zero and prints a warning to stderr.
   * @param name the name of the output within the Config
   * @param ix the index of the output
   */
  void compileOutput(const char* name, unsigned ix) throw();

  /**
   * Evaluates the output at the given index.
   * @param ix the output's index
   * @return the result of evaluating the function
   */
  float eval(unsigned ix) const throw();

  /**
   * Obtains a parameter and returns it.
   * If it is invalid, returns 0.
   */
  float parm(const char*) const throw();

  private:
  static void f_add(float*&,const execdat*&);
  static void f_sub(float*&,const execdat*&);
  static void f_mul(float*&,const execdat*&);
  static void f_div(float*&,const execdat*&);
  static void f_mod(float*&,const execdat*&);
  static void f_pow(float*&,const execdat*&);
  static void f_neg(float*&,const execdat*&);
  static void f_abs(float*&,const execdat*&);
  static void f_anm(float*&,const execdat*&);
  static void f_sqr(float*&,const execdat*&);
  static void f_rnd(float*&,const execdat*&);
  static void f_max(float*&,const execdat*&);
  static void f_min(float*&,const execdat*&);
  static void f_cos(float*&,const execdat*&);
  static void f_sin(float*&,const execdat*&);
  static void f_tan(float*&,const execdat*&);
  static void f_aco(float*&,const execdat*&);
  static void f_asi(float*&,const execdat*&);
  static void f_ata(float*&,const execdat*&);
  static void f_at2(float*&,const execdat*&);
  static void f_const(float*&,const execdat*&);
  static void f_input(float*&,const execdat*&);
  template<evaluator E, unsigned Arity>
  void c_fun  (std::vector<execdat>&, const libconfig::Setting&, unsigned);
  void c_const(std::vector<execdat>&, const libconfig::Setting&, unsigned);
  void c_input(std::vector<execdat>&, const libconfig::Setting&, unsigned);
  void c_nop  (std::vector<execdat>&, const libconfig::Setting&, unsigned);
  void compile(std::vector<execdat>&, const libconfig::Setting&, unsigned);
};

#endif /* CORTEX_HXX_ */
