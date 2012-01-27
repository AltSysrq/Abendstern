/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_core/aim_procedure.hxx
 */

/*
 * aim_procedure.cxx
 *
 *  Created on: 18.02.2011
 *      Author: jason
 */

#include <libconfig.h++>

#include <exception>
#include <stdexcept>
#include <cstring>

#include "aim_procedure.hxx"
#include "src/control/ai/aimod.hxx"
#include "src/control/ai/aictrl.hxx"

using namespace libconfig;
using namespace std;

#define MAX_CHILDREN 2048

/* Simple adapter class that allows using activate/deactivate
 * requiring classes to be used within procedures.
 */
class AIM_ProcedureChildActivationAdapter: public AIModule {
  AIModule* child;

  public:
  AIM_ProcedureChildActivationAdapter(AIControl* c, AIModule* m)
  : AIModule(c), child(m)
  { }

  ~AIM_ProcedureChildActivationAdapter() {
    delete child;
  }

  virtual void action() {
    child->activate();
    child->action();
    child->deactivate();
  }
};

AIM_Procedure::AIM_Procedure(AIControl* c, const Setting& s)
: AIModule(c)
{
  numChildren = s["modules"].getLength();
  if (numChildren > MAX_CHILDREN)
    throw range_error("Too many children for procedure");

  children = new Child[numChildren];
  memset(children, 0, sizeof(Child)*numChildren);
  unsigned totalWeight=0;

  try {
    //Load individual children
    for (unsigned i=0; i<numChildren; ++i) {
      const Setting& ch(s["modules"][i]);
      unsigned weight = ch.exists("weight")? (int)ch["weight"] : 1;
      AIModule* mod = controller.createAIModule(ch["module"], ch);

      if (!mod)
        throw runtime_error("Unknown module listed in procedure");

      if (mod->requiresActivateDeactivate())
        children[i].module = new AIM_ProcedureChildActivationAdapter(&controller, mod);
      else
        children[i].module = mod;
      children[i].cnt = weight;

      totalWeight += weight;
    }

    //Make sure we didn't get too big
    if (totalWeight > MAX_CHILDREN)
      throw range_error("Procedure too big");
  } catch (...) {
    for (unsigned i=0; i<numChildren; ++i)
      if (children[i].module)
        delete children[i].module;
    //Clean up, then forward exception
    delete[] children;
  }
}

AIM_Procedure::~AIM_Procedure() {
  for (unsigned i=0; i < numChildren; ++i)
    if (children[i].module)
      delete children[i].module;
  delete[] children;
}

void AIM_Procedure::action() {
  for (unsigned i=0; i<numChildren; ++i)
    for (unsigned j=0; j<children[i].cnt; ++j)
      controller.appendProcedure(children[i].module);
}

static AIModuleRegistrar<AIM_Procedure> registrar("core/procedure");
