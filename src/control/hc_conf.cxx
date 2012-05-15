/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/control/hc_conf.hxx
 */

//The GL/gl.h that ships with MSVS 2008 Pro isn't happy
//if SDL.h gets included before it (and it eventually
//*does* get included), so...
#include <GL/gl.h>
#include <SDL.h>
#include <utility>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <cstdio>
#include <libconfig.h++>

#include "src/globals.hxx"
#include "hc_conf.hxx"
#include "human_controller.hxx"
#include "joystick.hxx"

using namespace std;
using namespace libconfig;

namespace hc_conf {
  struct Binding {
    string name;
    DigitalAction base;
    ActionParam parm;
    ParamLimit limit;
    bool pair;

    Binding(string& _name, DigitalAction& _base, ActionParam _parm, ParamLimit _limit, bool _pair) :
      name(_name), base(_base), parm(_parm), limit(_limit), pair(_pair)
    {
      if (parm == CString) {
        char** str;
        if (pair) str = (char**)&base.datum.pair.second.ptr;
        else      str = (char**)&base.datum.ptr;
        if (*str) {
          const char* orig = *str;
          *str = new char[strlen(orig)+1];
          strcpy(*str, orig);
        }
      }
    }

    //More handling of string copies
    Binding(const Binding& other) :
      name(other.name), base(other.base), parm(other.parm),
      limit(other.limit), pair(other.pair)
    {
      if (parm == CString) {
        char** str;
        if (pair) str = (char**)&base.datum.pair.second.ptr;
        else      str = (char**)&base.datum.ptr;
        if (*str) {
          const char* orig = *str;
          *str = new char[strlen(orig)+1];
          strcpy(*str, orig);
        }
      }
    }
    //Delete string copies
    ~Binding() {
      if (parm == CString) {
        if (pair)
          delete[] static_cast<char*>(base.datum.pair.second.ptr);
        else
          delete[] static_cast<char*>(base.datum.ptr);
      }
    }
  };
  vector<Binding> bindings;

  const string mouseButtonNames[7] = {
    "left", "middle", "right", "up", "down", "x1", "x2"
  };

  void confAnalogue(Setting& setting, AnalogueAction& act) throw (ConfigException) {
    if      (0 == strcmp("none", setting["action"]))     act=action::noAction;
    else if (0 == strcmp("rotate", setting["action"]))   act=action::rotate;
    else if (0 == strcmp("throttle", setting["action"])) act=action::throttle;
    else if (0 == strcmp("accel", setting["action"]))    act=action::anaaccel;
    act.sensitivity = setting["sensitivity"];
    act.recentre = setting["recentre"];
  }

  void confDigital(Setting& setting, DigitalAction& act) throw (ConfigException) {
    //Handle bug in Windows 7 port of libconfig++
    const char* cname=setting["action"];
    string name(cname);
    Binding* bind=NULL;
    for (unsigned int i=0; i<bindings.size() && !bind; ++i)
      if (name.compare(bindings[i].name) == 0) bind = &bindings[i];

    if (!bind) {
      cerr << "WARNING: Ignoring unknown digital action \"" << name << "\"." << endl;
      return;
    }

    act=bind->base;
    act.repeat=setting["repeat"];
    if (setting.exists("value")) {
      switch (bind->parm) {
        case NoParam: break;
        case Integer: {
          int i=setting["value"];
          if (i < -bind->limit.intLimit || i > bind->limit.intLimit) {
            cerr << "WARNING: Ignoring out-of-range value " << i << " for action " << name <<
                    " (" << bind->limit.intLimit << " limit)" << endl;
            return;
          }
          if (bind->pair) act.datum.pair.second.cnt=i;
          else            act.datum.cnt=i;
        } break;
        case Float: {
          float f=setting["value"];
          if (f < -bind->limit.floatLimit || f > bind->limit.floatLimit) {
            cerr << "WARNING: Ignoring out-of-range value " << f << " for action " << name <<
                    " (" << bind->limit.floatLimit << " limit)" << endl;
            return;
          }
          if (bind->pair) act.datum.pair.second.amt=f;
          else               act.datum.amt=f;
        } break;
        case CString: {
          const char* str=setting["value"];
          //Copy the string. Windows seems to have issues with using the reference,
          //which is *supposed* to be safe
          char* nstr=new char[strlen(str)+1];
          strcpy(nstr, str);
          if (bind->pair) act.datum.pair.second.ptr=nstr;
          else            act.datum.ptr=nstr;
        } break;
      }
    }
  }

  void configure(HumanController* hc, const char* section) throw (ConfigException) {
    Setting& setting(conf["conf"][section]);

    //Analogue settings for the mouse are hardwired
    confAnalogue(setting["analogue"]["vert"], hc->mouseVert);
    confAnalogue(setting["analogue"]["horiz"], hc->mouseHoriz);

    //Joysticks
    for (unsigned i = 0; i < hc->joysticks.size(); ++i) {
      char joyname_[] = "joy_XXXX";
      sprintf(joyname_, "joy_%d", i);
      //ISO C++ says that implicit char[]->string == char[]->char* in priority,
      //so G++ won't compile joyname_ used as an index (note the casts with the
      //lower indices). To save those casts, explicitly store into a const
      //char* variable.
      const char* joyname = joyname_;
      if (!setting.exists(joyname)) continue;

      //Normal axes
      for (unsigned j=0; j<hc->joysticks[i].axes[joystick::Axis].size(); ++j) {
        char axisname[] = "axis_XXXX";
        sprintf(axisname, "axis_%d", j);
        if (setting[joyname].exists(axisname)) {
          confAnalogue(setting[joyname][(const char*)axisname],
                       hc->joysticks[i].axes[joystick::Axis][j]);
        }
      }

      //Balls
      for (unsigned j=0; j<hc->joysticks[i].axes[joystick::BallX].size(); ++j) {
        char axisname[] = "ball_x_XXXX";
        sprintf(axisname, "ball_x_%d", j);
        if (setting[joyname].exists(axisname)) {
          confAnalogue(setting[joyname][(const char*)axisname],
                       hc->joysticks[i].axes[joystick::BallX][j]);
        }
      }
      for (unsigned j=0; j<hc->joysticks[i].axes[joystick::BallY].size(); ++j) {
        char axisname[] = "ball_y_XXXX";
        sprintf(axisname, "ball_y_%d", j);
        if (setting[joyname].exists(axisname)) {
          confAnalogue(setting[joyname][(const char*)axisname],
                       hc->joysticks[i].axes[joystick::BallY][j]);
        }
      }

      //Normal buttons
      for (unsigned j=0; j<hc->joysticks[i].buttons[joystick::Button].size();
           ++j) {
        char buttonname[] = "button_XXXX";
        sprintf(buttonname, "button_%d", j);
        if (setting[joyname].exists(buttonname)) {
          confDigital(setting[joyname][(const char*)buttonname],
                      hc->joysticks[i].buttons[joystick::Button][j]);
        }
      }
      //Hats
      for (unsigned hat = (unsigned)joystick::HatUp;
           hat <= (unsigned)joystick::HatRight;
           ++hat) {
        for (unsigned j=0; j < hc->joysticks[i].buttons[hat].size(); ++j) {
          char buttonname[] = "hat_RIGHT_XXXX";
          sprintf(buttonname, "hat_%s_%d",
                  (hat == joystick::HatUp?    "up"    :
                   hat == joystick::HatDown?  "down"  :
                   hat == joystick::HatLeft?  "left"  :
                                              "right"), j);
          if (setting[joyname].exists(buttonname)) {
            confDigital(setting[joyname][(const char*)buttonname],
                        hc->joysticks[i].buttons[hat][j]);
          }
        }
      }
    }

    for (unsigned int i=0; i<=SDL_BUTTON_X2-1; ++i)
      if (setting["mouse"].exists(mouseButtonNames[i]))
        confDigital(setting["mouse"][mouseButtonNames[i]], hc->mouseButton[i+1]);


    for (unsigned int i=0; i<=SDLK_LAST; ++i) {
      char name[]="key_XXX  "; //Leave extra space in case a sym >=1000 is added
      sprintf(name, "key_%03d", i);
      if (setting["keyboard"].exists(name))
        confDigital(setting["keyboard"][(const char*)name], hc->keyboard[i]);
    }
  }

  void bind(DigitalAction& act, const char* cname, ActionParam parm, bool parmPaired, ParamLimit limit) {
    string name(cname);
    //Remove if exists
    for (unsigned int i=0; i<bindings.size(); ++i)
      if (bindings[i].name.compare(name) == 0)
        bindings.erase(bindings.begin() + (i--));

    Binding bind (
      name,
      act,
      parm,
      limit,
      parmPaired
    );
    bindings.push_back(bind);
  }

  void clear() {
    bindings.clear();
  }
};
