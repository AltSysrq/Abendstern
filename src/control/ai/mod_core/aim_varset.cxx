/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/ai/mod_core/aim_varset.hxx
 */

/*
 * aim_varset.cxx
 *
 *  Created on: 19.02.2011
 *      Author: jason
 */

#include <libconfig.h++>

#include <vector>
#include <utility>
#include <string>
#include <cstring>
#include <exception>
#include <stdexcept>

#include "aim_varset.hxx"

using namespace std;
using namespace libconfig;

AIM_Varset::AIM_Varset(AIControl* c, const Setting& s)
: AIModule(c)
{
  for (unsigned i=0; i<(unsigned)s.getLength(); ++i) {
    const Setting& v(s[i]);
    const char* name = v.getName();
    if (0 == strcmp("module", name)) continue;
    if (0 == strcmp("weight", name)) continue;

    if (*name != 'g' && *name != 's')
      throw runtime_error("Invalid parm class in configuration for core/set");
    if (!name[1])
      throw runtime_error("Invalid empty parm in configuration for core/set");

    AIControl::Variable var;

    switch (v.getType()) {
      case Setting::TypeBoolean:
        var.type = AIControl::Variable::Bool;
        var.asBool = (bool)v;
        break;
      case Setting::TypeInt:
      case Setting::TypeInt64:
        var.type = AIControl::Variable::Int;
        var.asInt = (int)v;
        break;
      case Setting::TypeFloat:
        var.type = AIControl::Variable::Float;
        var.asFloat = (float)v;
        break;
      case Setting::TypeString:
        var.type = AIControl::Variable::String;
        var.asString = (const char*)v;
        break;
      default:
        throw runtime_error("Invalid type for parm in core/set");
    }

    vars.push_back(make_pair(string(name), var));
  }
}

void AIM_Varset::action() {
  for (unsigned i=0; i<vars.size(); ++i) {
    AIControl::Variable& var(vars[i].first[0] == 'g'?
                             controller[vars[i].first.c_str()+1]
                            :controller(vars[i].first.c_str()+1));
    var = vars[i].second;
  }
}

static AIModuleRegistrar<AIM_Varset> registrar("core/set");
