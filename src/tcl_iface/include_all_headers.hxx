/**
 * @file
 * @author Jason Lingle
 * @brief Includes all headers needed by the C++-Tcl bridge
 */

#ifndef INCLUDE_ALL_HEADERS_HXX_
#define INCLUDE_ALL_HEADERS_HXX_

#include <map>
#include <set>
#include <vector>
#include <string>
#include <cstring>
#include <utility>

#include <cstdio>
#include <iostream>
#include <cstdlib>

#include <SDL.h>
#include <GL/gl.h>
#include <libconfig.h++>

#include <tcl.h>

#include "src/exit_conditions.hxx"
#include "src/globals.hxx"
#include "src/test_state.hxx"
#include "src/audio/ship_mixer.hxx"
#include "src/background/background.hxx"
#include "src/background/background_object.hxx"
#include "src/background/explosion.hxx"
#include "src/background/explosion_pool.hxx"
#include "src/background/nebula.hxx"
#include "src/background/old_style_explosion.hxx"
#include "src/background/planet.hxx"
#include "src/background/star_field.hxx"
#include "src/background/star.hxx"
#include "src/core/aobject.hxx"
#include "src/core/game_state.hxx"
#include "src/core/init_state.hxx"
#include "src/core/lxn.hxx"
#include "src/camera/camera.hxx"
#include "src/camera/dynamic_camera.hxx"
#include "src/camera/effects_handler.hxx"
#include "src/camera/fixed_camera.hxx"
#include "src/camera/forwarding_effects_handler.hxx"
#include "src/camera/hud.hxx"
#include "src/camera/spectator.hxx"
#include "src/control/controller.hxx"
#include "src/control/genetic_ai.hxx"
#include "src/control/hc_conf.hxx"
#include "src/control/human_controller.hxx"
#include "src/control/ai/aictrl.hxx"
#include "src/control/genai/genai.hxx"
#include "src/graphics/acsgi.hxx"
#include "src/graphics/asgi.hxx"
#include "src/graphics/font.hxx"
#include "src/graphics/imgload.hxx"
#include "src/net/crypto.hxx"
#include "src/secondary/global_chat.hxx"
#include "src/secondary/light_trail.hxx"
#include "src/secondary/namegen.hxx"
#include "src/secondary/planet_generator.hxx"
#include "src/secondary/validation.hxx"
#include "src/ship/everything.hxx"
#include "src/ship/editor/manipulator.hxx"
#include "src/sim/blast.hxx"
#include "src/sim/collision.hxx"
#include "src/sim/game_env.hxx"
#include "src/sim/game_field.hxx"
#include "src/sim/game_object.hxx"
#include "src/sim/objdl.hxx"
#include "src/weapon/energy_charge.hxx"
#include "src/weapon/magneto_bomb.hxx"
#include "src/weapon/missile.hxx"
#include "src/weapon/monophasic_energy_pulse.hxx"
#include "src/weapon/plasma_burst.hxx"
#include "src/weapon/semiguided_bomb.hxx"
#include "array.hxx"
#include "dynfun.hxx"
#include "common_keyboard_client.hxx"
#include "slave_thread.hxx"
#include "square_icon.hxx"

using namespace std;
using namespace libconfig;

/* No, this is not the right "class Peer".
 * But it /does/ allow us to get a typeid, which we track
 * statically, so it's ok to have the wrong thing.
 *
 * It seems that MSVC++ linker CAN tell the difference between
 * non-identical classes of the same name, despite not
 * being able to tell the difference between non-identical
 * anonymous classes... fortunately, it also allows
 * taking the typeid of a class the compiler can't see.
 */
#ifndef WIN32
struct Peer { int a; };
#endif

#endif /*INCLUDE_ALL_HEADERS_HXX_*/
