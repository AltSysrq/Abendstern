# Critical binding
cxx
fun void {cppDelete {c++ delete}} string string

predecclass Ship NetworkConnection InputNetworkGeraet

cxx src/graphics/asgi.hxx
enum asgi::Primitive {} \
  {asgi::Points        GL_POINTS} \
  {asgi::Lines         GL_LINES} \
  {asgi::LineStrip     GL_LINE_STRIP} \
  {asgi::LineLoop      GL_LINE_LOOP} \
  {asgi::Triangles     GL_TRIANGLES} \
  {asgi::TriangleStrip GL_TRIANGLE_STRIP} \
  {asgi::TriangleFan   GL_TRIANGLE_FAN} \
  {asgi::Quads         GL_QUADS}

fun void {asgi::begin           glBegin} asgi::Primitive
fun void {asgi::end             glEnd}
fun void {asgi::vertex          glVertex} float float
fun void {asgi::colour          glColour} float float float float
fun void {asgi::pushMatrix      glPushMatrix}
fun void {asgi::popMatrix       glPopMatrix}
fun void {asgi::loadIdentity    glLoadIdentity}
fun void {asgi::translate       glTranslate} float float
fun void {asgi::rotate          glRotate} float
fun void {asgi::scale           glScale} float float
fun void {asgi::uscale          glUScale} float
unsafe {
  fun void {asgi::reset         glReset}
}

cxx src/graphics/acsgi.hxx
fun void acsgi_begin
fun void acsgi_end
fun void acsgi_draw
fun void acsgi_textNormal bool
fun void {acsgid_begin          cglBegin} asgi::Primitive
fun void {acsgid_end            cglEnd}
fun void {acsgid_vertex         cglVertex} float float
fun void {acsgid_colour         cglColour} float float float float
fun void {acsgid_pushMatrix     cglPushMatrix}
fun void {acsgid_popMatrix      cglPopMatrix}
fun void {acsgid_loadIdentity   cglLoadIdentity}
fun void {acsgid_translate      cglTranslate} float float
fun void {acsgid_rotate         cglRotate} float
fun void {acsgid_scale          cglScale} float float
fun void {acsgid_uscale         cglUScale} float
fun void {acsgid_text           cglText} cstr float float

cxx
##BEGIN -- SDL Events
set knownTypes(Uint8) [new IntType Uint8]
set knownTypes(Uint16) [new IntType Uint16]
set knownTypes(Sint16) [new IntType Sint16]

# I don't want to type in SDLK_... for everything.
# Process a more inteligent way (and reprefix
# everything with just k_, and lowercase it too).
# But first, a couple things that need more sane renaming
# UNKNOWN must be the first item on the list, though, since
# this is an open enumeration
set sdlklist {
  {SDLK_UNKNOWN k_unknown}
  {SDLK_RETURN k_enter}
  {SDLK_SCROLLOCK k_scrolllock}
}
foreach sym {
  backspace
  tab
  clear
  pause
  escape
  space
  exclaim
  quotedbl
  hash
  dollar
  ampersand
  quote
  leftparen
  rightparen
  asterisk
  plus
  comma
  minus
  period
  slash
  0 1 2 3 4 5 6 7 8 9
  colon
  semicolon
  less
  equals
  greater
  question
  at
  leftbracket
  backslash
  rightbracket
  caret
  underscore
  backquote
  delete
  kp0 kp1 kp2 kp3 kp4 kp5 kp6 kp7 kp8 kp9
  kp_period
  kp_divide
  kp_multiply
  kp_minus
  kp_plus
  kp_enter
  kp_equals
  up
  down
  right
  left
  insert
  home
  end
  pageup
  pagedown
  f1 f2 f3 f4
  f5 f6 f7 f8
  f9 f10 f11 f12
  f13 f14 f15
  numlock
  capslock
  rshift
  lshift
  rctrl
  lctrl
  ralt
  lalt
  rmeta
  lmeta
  lsuper
  rsuper
  mode
  compose
  help
  print
  sysreq
  break
  menu
  power
  euro
  undo
} {
  lappend sdlklist "SDLK_[string toupper $sym] k_$sym"
}
# Lowercase letters
foreach l {a b c d e f g h i j k l m n o p q r s t u v w x y z} {
  lappend sdlklist "SDLK_$l k_$l"
}
# World0..95
for {set i 0} {$i <= 95} {incr i} {
  lappend sdlklist "SDLK_WORLD_$i k_world_$i"
}

eval "openenum SDLKey {} $sdlklist"
const SDLK_LAST unsigned
fun unsigned SDLKeyToInt SDLKey

# We unfortunately have no choice but to treate the
# SDLMod enum as an integer, since it's ORed
set knownTypes(SDLMod) [new IntType SDLMod]
foreach mod {
  none
  lshift rshift
  lctrl rctrl
  lalt ralt
  lmeta rmeta
  num caps
  mode reserved
} {
  const "[string toupper "kmod_$mod"] $mod" SDLMod
}

class foreign SDL_keysym {} {
  var scancode Uint8
  var sym SDLKey
  var mod SDLMod
  var unicode Uint16;
}

enum {Uint8 SdlKType} {} {SDL_KEYDOWN DOWN} {SDL_KEYUP UP}
enum {Uint8 SdlKState} {} {SDL_PRESSED PRESSED} {SDL_RELEASED RELEASED}
class foreign SDL_KeyboardEvent {} {
  var type SdlKType
  var state SdlKState
  var keysym SDL_keysym
}

foreach b {1 2 3 4 5} {const "SDL_BUTTON($b) SDL_BUTTON_$b" Uint8}

unsafe {
  fun SDLMod SDL_GetModState
  const SDL_DEFAULT_REPEAT_INTERVAL int
  const SDL_DEFAULT_REPEAT_DELAY int
  fun void SDL_EnableKeyRepeat int int
}

class foreign SDL_MouseMotionEvent {} {
  var type Uint8
  var state Uint8
  var x Uint16
  var y Uint16
  var xrel Sint16
  var yrel Sint16
}

enum {Uint8 SdlMType} {} {SDL_MOUSEBUTTONDOWN DOWN} {SDL_MOUSEBUTTONUP UP}
enum {Uint8 SdlMButton} {} {SDL_BUTTON_LEFT mb_left}   {SDL_BUTTON_MIDDLE mb_mid} \
                           {SDL_BUTTON_RIGHT mb_right} {SDL_BUTTON_WHEELUP mb_wup} \
                           {SDL_BUTTON_WHEELDOWN mb_wdown}
class foreign SDL_MouseButtonEvent {} {
  var type SdlMType
  var which Uint8
  var button SdlMButton
  var state SdlKState
  var x Uint16
  var y Uint16
}

fun cstr SDL_GetKeyName SDLKey

##END -- SDL Events

##BEGIN -- Misc SDL stuff
unsafe {
  fun void SDL_WarpMouse Uint16 Uint16
  fun Uint32 SDL_GetTicks
}
##END -- Misc SDL stuff

##BEGIN to EOF -- Abendstern
cxx
newPairType float float

cxx src/abendstern.hxx
enum AbendsternGLType {} AGLT14 AGLT21 AGLT32
unsafe {
  const preliminaryRunMode bool
  const THIS_GL_TYPE AbendsternGLType
  var recommendedGLType AbendsternGLType
  fun void exitPreliminaryRunMode
}

cxx src/graphics/font.hxx
class final Font {} {
  unsafe {
    constructor default cstr float bool
  }
  fun float {width charWidth} const char
  fun float width const cstr
  fun float getHeight const
  fun float getRise const
  fun float getDip const
  fun void preDraw const
  fun void postDraw const
  fun void {draw drawCh} const char float float
  fun void {draw drawStr} const cstr float float
}

const sysfont Font*
const sysfontStipple Font*

cxx src/core/game_state.hxx
class abstract-extendable GameState {} {
  constructor default
  fun {GameState* steal} update purevirtual float
  fun void draw purevirtual
  fun void configureGL virtual
  fun void keyboard virtual SDL_KeyboardEvent*
  fun void motion virtual SDL_MouseMotionEvent*
  fun void mouseButton virtual SDL_MouseButtonEvent*
}

cxx src/core/init_state.hxx
unsafe {
  class final InitState GameState {
    constructor default
  }
}

cxx src/test_state.hxx
var {test_state::gameClass gameClass} cstr copy
var {test_state::humanShip humanShip} int
enum {test_state::Mode TestStateMode} TSM \
  {test_state::StandardTeam StandardTeam} \
  {test_state::HomogeneousTeam HomogeneousTeam} \
  {test_state::HeterogeneousTeam HeterogeneousTeam} \
  {test_state::FreeForAll FreeForAll} \
  {test_state::LastManStanding LastManStanding} \
  {test_state::ManVsWorld ManVsWorld}
enum {test_state::Background TestStateBackground} TSB \
  {test_state::StarField StarField} {test_state::Planet Planet} \
  {test_state::Nebula Nebula}
var {test_state::mode testStateMode} TestStateMode
var {test_state::size testStateSize} int

unsafe {
  class final TestState GameState {
    constructor default TestStateBackground
    fun {GameState* steal} update {} float
    fun void draw
  }
}

cxx src/sim/game_object.hxx src/sim/collision.hxx src/sim/game_field.hxx
class final GameField {}
enum GameObject::Classification GOClass Generic {ClassShip Ship}\
     LightWeapon HeavyWeapon
enum CollisionResult {} NoCollision UnlikelyCollision \
                        MaybeCollision YesCollision
class abstract-extendable GameObject {} {
  const isRemote bool
  var isExportable bool
  var tag string
  var ignoreNetworkTag bool
  const field GameField* {} protected
  const {ci.isDead isDead} bool
  # Can't do collisionBounds yet
  var x float {} protected
  var y float {} protected
  var vx float {} protected
  var vy float {} protected
  var includeInCollisionDetection bool {} protected
  var classification GameObject::Classification {} protected
  var decorative bool {} protected

  constructor default GameField*
  constructor position GameField* float float
  constructor velocity GameField* float float float float
  fun bool update {purevirtual noth} float
  fun void draw {purevirtual noth}
  # No collision support yet
  fun float getX const
  fun float getY const
  fun float getVX const
  fun float getVY const
  fun GameField* getField const
  fun bool isDecorative const
  fun GameObject::Classification getClassification const
  fun void okToDecorate
  fun void teleport {virtual noth} float float float
  fun float getRotation {virtual noth const}
  fun float getRadius {purevirtual noth const}
  fun bool isCollideable {virtual noth const}
  fun CollisionResult checkCollision {virtual noth} GameObject*
  fun void del
}

cxx src/camera/effects_handler.hxx
class extendable EffectsHandler {}

cxx src/sim/game_field.hxx src/camera/effects_handler.hxx
class final GameField {} {
  const fieldClock Uint32
  var width float
  var height float
  var effects EffectsHandler* steal
  var perfectRadar bool
  constructor default float float
  fun void update {} float
  fun void draw
  fun GameObject* {operator[] at} const \
    {unsigned {check ok=(val < parent->size());}}
  fun unsigned size const
  fun void add {} {GameObject* steal {check ok=val;}}
  fun void addBegin {} {GameObject* steal {check ok=val;}}
  fun void remove {} {GameObject* yield}
  fun void inject {} {GameObject* steal {check ok=val;}}
  fun void clear noth
  fun void updateBoundaries noth
}

cxx src/sim/blast.hxx
class final Blast GameObject {
  const blame unsigned
  constructor default     {GameField* {check ok=val;}} unsigned float float float float
  constructor withDirect  {GameField* {check ok=val;}} unsigned float float float float bool
  constructor withSize    {GameField* {check ok=val;}} unsigned float float float float bool float
  constructor withADC     {GameField* {check ok=val;}} unsigned float float float float bool float bool
  constructor withDecor   {GameField* {check ok=val;}} unsigned float float float float bool float bool bool
  constructor withDamage  {GameField* {check ok=val;}} unsigned float float float float bool float bool bool bool
  constructor nonDamageCopy {Blast* {check ok=val;}} bool
  fun bool update noth float
  fun void draw noth
  fun float getFalloff
  fun float getStrength
  fun float {getStrength getStrengthAt} {} float
  fun float {getStrength getStrengthUpon} {} {GameObject* {check ok=val;}}
  fun float getSize {}
  fun bool isDirect
  fun bool causesDamage
}

# Tcl needs to know about ANY type of GameObject that can come into
# existence, even if Tcl will never need to mess with it
# So include these visual effects
cxx src/ship/auxobj/cell_fragment.hxx
class final CellFragment GameObject {
  fun bool update {} float
  fun void draw
  fun float getRadius
}
cxx src/ship/auxobj/plasma_fire.hxx
class final PlasmaFire GameObject {
  fun bool update {} float
  fun void draw
  fun float getRadius
}

cxx src/background/explosion.hxx src/sim/game_field.hxx
enum ExplosionType {} {*}{
  Explosion::Simple
  Explosion::Spark
  Explosion::BigSpark
  Explosion::Sparkle
  Explosion::Incursion
  Explosion::Flame
  Explosion::Invisible
}
unsafe {
  class final Explosion GameObject {
    var hungry bool
    constructor stationary    GameField* ExplosionType float float float float float float float float
    constructor velocity      GameField* ExplosionType float float float float float float float float float float
    constructor smeared       GameField* ExplosionType float float float float float float float float float float float float
    constructor by            GameField* ExplosionType float float float float float float GameObject*
    # Pretend the update and draw functions are unimplemented
    # They do nothing useful now, so it's an error to call them
    # Same goes for other things from GameObject
    # (in many ways, an Explosion isn't a GameObject anymore...)
    fun void multiExplosion {} int
    fun float getColourR
    fun float getColourG
    fun float getColourB
    fun float getSize
    fun float getDensity
  }
}

cxx src/background/old_style_explosion.hxx \
    src/background/explosion.hxx
unsafe {
  class final OldStyleExplosion GameObject {
    constructor default Explosion*
    fun bool update {} float
    fun void draw
    fun float getRadius
  }
}

cxx src/secondary/light_trail.hxx src/sim/game_field.hxx
unsafe {
  class final LightTrail GameObject {
    constructor default GameField* float int float float float float float float float float float float
    fun bool update {} float
    fun void draw
    fun float getRadius
    fun void setWidth {} float
    fun void emit {} float float float float
  }
}

cxx src/ship/ship.hxx
class abstract-extendable ShipSystem {}
class final Shield {}
class final Cell {}
class abstract-extendable Controller {}
class final Ship GameObject
newFunType void Ship* bool

class final radar_t {} {
  constructor default
}

cxx src/ship/ship.hxx src/sim/game_field.hxx src/control/controller.hxx \
    src/camera/effects_handler.hxx src/ship/cell/cell.hxx \
    src/ship/ship_image_renderer.hxx
enum Ship::Category {} \
    {Ship::Swarm 	Swarm} \
    {Ship::Interceptor	Interceptor} \
    {Ship::Fighter	Fighter} \
    {Ship::Attacker	Attacker} \
    {Ship::Subcapital	Subcapital} \
    {Ship::Defender	Defender}
fun unsigned shipCategoryToInt Ship::Category
class final Ship GameObject {
  unsafe {
    var controller Controller* steal
    # This used to be steal, since the Ship created its own
    # ForwardingEffectsHandler. It no longer does (because this was impossible
    # to manage correctly), so ownership is now whoever created the object.
    var effects EffectsHandler*
    var shipExistenceFailure fun<void:Ship*,bool>::fun_t*
  }

  fun pair<float,float> cellCoord static {Ship* {check ok=val;}} {Cell* {check ok=val;}}
  fun pair<float,float> {cellCoord cellSubCoord} static {Ship* {check ok=val;}} {Cell* {check ok=val;}} float float
  var insignia unsigned
  var blame unsigned
  var score int
  var playerScore int
  var damageMultiplier float
  const diedSpontaneously bool
  const typeName string

  unsafe {
    constructor default GameField*
  }

  fun void refreshUpdates
  fun bool update noth float
  unsafe {
    fun void draw noth
  }
  fun float getVRotation {const noth}
  fun float getMass noth
  fun void {glSetColour glSetColour0} noth
  fun void {glSetColour glSetColour1} noth float
  fun void {glSetColour glSetColour2} noth float float
  fun void setThrust noth float
  fun float getThrust
  fun float getTrueThrust
  fun void setThrustOn noth bool
  fun bool isThrustOn
  fun void setBrakeOn noth bool
  fun bool isBrakeOn
  fun void configureEngines noth bool bool float
  fun void {configureEngines configureEngines2} noth bool bool
  fun float getAcceleration
  fun float getRotationRate
  fun float getRotationAccel
  fun float getRadius
  fun float getPowerUsagePercent
  fun unsigned getPowerSupply
  fun unsigned getPowerDrain
  fun float getCurrentCapacitance
  fun unsigned getMaximumCapacitance
  fun float getCapacitancePercent
  fun void setColour noth float float float
  fun void destroyGraphicsInfo noth
  fun float getColourR
  fun float getColourG
  fun float getColourB
  fun bool drawPower noth float
  fun float getReinforcement
  unsafe {
    fun void setReinforcement noth float
    fun void enableSoundEffects
  }
  fun bool hasPower
  fun void spontaneouslyDie
  fun float getCoolingMult
  unsafe {
    fun pair<float,float> getCellVelocity {const noth} {Cell* {check ok=val;}}
  }
  fun void spin noth float
  unsafe {
    fun void startTest
    fun float endTest
  }
  fun void applyCollision noth float float float float float
  fun unsigned cellCount

  fun radar_t* getRadar
  fun void setRadar {} radar_t*

  fun cstr getDeathAttributions
  fun Ship::Category categorise
}
fun Ship::Category {Ship::categorise Ship_categorise} cstr

class final ShipImageRenderer {} {
  fun bool renderNext
  fun bool save const cstr
}
fun {ShipImageRenderer* steal} \
    {ShipImageRenderer::create ShipImageRenderer_create} Ship*

cxx src/ship/ship.hxx src/ship/shipio.hxx src/ship/cell/cell.hxx \
    src/camera/effects_handler.hxx src/sim/game_field.hxx
fun cstr verify {Ship* {check ok=val;}}
unsafe {
  fun {Ship* steal} loadShip GameField* cstr
  fun void saveShip Ship* cstr
}

enum Weapon {} Weapon_EnergyCharge Weapon_MagnetoBomb Weapon_PlasmaBurst Weapon_SGBomb
enum TargettingMode {} TargettingMode_NA TargettingMode_None TargettingMode_Standard \
  TargettingMode_Bridge TargettingMode_Power TargettingMode_Weapon TargettingMode_Engine \
  TargettingMode_Shield TargettingMode_Cell

cxx src/ship/auxobj/shield.hxx
unsafe {
  class final Shield {} {
    fun void update {} float
    fun void updateDist
    fun void draw
    fun float getRadius
    fun bool collideWith {} GameObject*
    fun float getStrength
    fun float getStability
    fun Ship* getShip
    fun void drawForHUD {} float float float float
  }
}

cxx src/ship/insignia.hxx
enum Alliance A Neutral Allies Enemies
fun unsigned insignia cstr
unsafe {
  fun void clear_insignias
}
fun Alliance getAlliance unsigned unsigned
unsafe {
  fun void setAlliance Alliance unsigned unsigned
}

cxx src/weapon/energy_charge.hxx \
    src/sim/game_field.hxx src/ship/ship.hxx
class final EnergyCharge GameObject {
  constructor default {GameField* {check ok=val;}} {Ship* {check ok=val;}} float float float float
  fun bool update {virtual noth} float
  fun void draw
  fun float getRadius
  fun float getIntensity
  fun void explode {} {GameObject* {check ok=val;}}
}

cxx src/weapon/magneto_bomb.hxx \
    src/sim/game_field.hxx src/ship/ship.hxx
class final MagnetoBomb GameObject {
  constructor default {GameField* {check ok=val;}} float float float float float {Ship* {check ok=val;}}
  fun bool update {virtual noth} float
  fun void draw
  fun float getRadius
  fun float getPower
  fun void simulateFailure
}

cxx src/weapon/semiguided_bomb.hxx \
    src/sim/game_field.hxx src/ship/ship.hxx
class final SemiguidedBomb MagnetoBomb {
  constructor default {GameField* {check ok=val;}} float float float float float {Ship* {check ok=val;}}
}

cxx src/weapon/plasma_burst.hxx \
    src/ship/ship.hxx src/sim/game_field.hxx
class final PlasmaBurst GameObject {
  constructor default {GameField* {check ok=val;}} {Ship* {check ok=val;}} float float float float float float
  fun bool update {virtual noth} float
  fun void draw
  fun float getRadius
  fun float getMass
}

cxx src/weapon/monophasic_energy_pulse.hxx \
    src/ship/ship.hxx src/sim/game_field.hxx
class final MonophasicEnergyPulse GameObject {
  constructor default {GameField* {check ok=val;}} {Ship* {check ok=val;}} float float float int
}

cxx src/weapon/missile.hxx src/ship/ship.hxx src/sim/game_field.hxx
class final Missile GameObject {
  constructor default {GameField* {check ok=val;}} int float float float float Ship* GameObject*
}

cxx src/camera/effects_handler.hxx src/background/explosion.hxx
class extendable EffectsHandler {} {
  constructor default
  fun void impact {virtual noth} float
#  fun void engines {virtual noth} float Ship*
  fun void explode {virtual noth} Explosion*
}
const nullEffectsHandler EffectsHandler

cxx src/background/background.hxx src/sim/game_object.hxx
unsafe {
  class final Background EffectsHandler {
    fun void draw
    fun void update {} float
    fun void updateReference {} GameObject* bool
    fun void repopulate
  }
}

cxx src/background/planet.hxx
unsafe {
  class final Planet Background {
    constructor default GameObject* GameField* cstr cstr float float float float
  }
}

cxx src/background/star_field.hxx \
    src/sim/game_object.hxx src/sim/game_field.hxx
unsafe {
  class final StarField Background {
    constructor default GameObject* GameField* int
  }

  fun void initStarLists
}

cxx src/background/nebula.hxx src/sim/game_object.hxx src/sim/game_field.hxx
unsafe {
  class final Nebula Background {
    constructor default GameObject* GameField* float float float float float
    fun cstr setFlowEquation noth cstr cstr bool
    fun cstr setPressureEquation noth cstr bool
    fun void setPressureResetTime noth float
    fun float getPressureResetTime const
    fun void setVelocityResetTime noth float
    fun float getVelocityResetTime const
    fun void setForceMultiplier noth float
  }
}

cxx src/camera/camera.hxx
unsafe {
  class abstract-extendable Camera EffectsHandler {
    constructor default GameObject*
    var reference GameObject* {} protected
    fun void doSetup {purevirtual noth protected}
    fun void update {virtual noth} float
    fun void drawOverlays {virtual noth}
    fun void reset {virtual noth}
    fun void setup {} bool
  }
}

cxx src/camera/dynamic_camera.hxx
unsafe {
  enum DynamicCamera::RotateMode DCRM \
    {DynamicCamera::None None} \
    {DynamicCamera::Direction Direction} \
    {DynamicCamera::Velocity Velocity}
  class final DynamicCamera Camera {
    constructor default GameObject* GameField*
    fun void update {} float
    fun void doSetup protected
    fun void reset
    fun float getZoom const
    fun void setZoom {} float
    fun DynamicCamera::RotateMode getRotateMode const
    fun void setRotateMode {} DynamicCamera::RotateMode
    fun float getLookAhead const
    fun void setLookAhead {} float
    fun float getVisualRotation {}
    fun void hc_conf_bind
  }
}

cxx src/camera/fixed_camera.hxx
unsafe {
  class final FixedCamera Camera {
    constructor default GameObject*
  }
}

cxx src/control/controller.hxx src/ship/ship.hxx
unsafe {
  class abstract-extendable Controller AObject {
    const ship Ship*
    constructor default Ship*
    fun void update {purevirtual noth} float
    fun void damage {virtual noth} float float float
    fun void otherShipDied {virtual noth} Ship*
    fun void notifyScore {virtual noth} int
  }
}

cxx src/control/human_controller.hxx src/ship/ship.hxx
unsafe {
  class final HumanController Controller {
    constructor default Ship*
    fun void hc_conf_bind
    fun void update {} float
    fun void motion {} SDL_MouseMotionEvent*
    fun void button {} SDL_MouseButtonEvent*
    fun void key    {} SDL_KeyboardEvent*
  }
  const isCompositionBufferInUse bool
  var compositionBufferPrefix string
}

cxx src/control/hc_conf.hxx
unsafe {
  fun void {hc_conf::configure hc_conf_configure} HumanController* cstr
  fun void {hc_conf::clear hc_conf_clear}
}

cxx src/control/ai/aictrl.hxx src/ship/ship.hxx
unsafe {
  class final AIControl Controller {
    constructor default Ship* cstr cstr
    fun void update {} float
  }
}

cxx src/control/genai/genai.hxx src/control/genetic_ai.hxx \
    src/ship/ship.hxx
unsafe {
  class final GeneticAI Controller {
    constructor default Ship*
    fun void update {} float
    const species unsigned
    const generation unsigned
    const instance unsigned
    const failed bool
  }
  fun void calculateGeneticAIFunctionCosts
  class final GenAI Controller {
    fun void update {} float
    const species unsigned
    const generation unsigned
    fun string getScores const
  }
  fun GenAI* {GenAI::makeGenAI GenAI_make} Ship*
}

cxx src/tcl_iface/common_keyboard_client.hxx
unsafe {
  class extendable CommonKeyboardClient AObject {
    constructor default
    fun void exit virtual
    fun void slow virtual
    fun void fast virtual
    fun void halt virtual
    fun void frameXframe virtual
    fun void statsOn virtual
    fun void statsOff virtual
    fun void hc_conf_bind
  }
}

cxx src/camera/forwarding_effects_handler.hxx
class final ForwardingEffectsHandler EffectsHandler {
  constructor default Ship*
}

cxx src/camera/spectator.hxx src/ship/ship.hxx src/sim/game_field.hxx
class final Spectator GameObject {
  constructor default Ship*
  constructor explicit Ship* bool
  constructor empty GameField*
  fun void nextReference
  fun void requireInsignia {} unsigned
  fun void kill
  fun Ship* getReference
}

cxx src/sim/game_env.hxx src/background/background.hxx src/camera/camera.hxx
unsafe {
  class final GameEnv {} {
    constructor customCamera {Camera* steal} float float
    constructor default float float
    fun GameObject* getReference const
    fun void setReference {} GameObject* bool
    fun void update {} float
    fun void draw {}
    fun GameField* getField
    var cam Camera* steal
    var stars Background* steal
  }
}

cxx src/ship/editor/manipulator.hxx src/ship/ship.hxx src/ship/cell/cell.hxx
unsafe {
  class final Manipulator {} {
    constructor default
    fun void update
    fun void draw
    fun void primaryDown {} float float
    fun void primaryUp {} float float
    fun void secondaryDown {} float float
    fun void secondaryUp {} float float
    fun void scrollUp {} float float
    fun void scrollDown {} float float
    fun void motion {} float float float float
    fun void resetView
    fun void pushUndo
    fun void popUndo
    fun void commitUndo
    fun void deactivateMode
    fun void activateMode
    fun void addToHistory
    fun void revertToHistory {} unsigned
    fun cstr reloadShip
    fun void deleteShip
    fun void copyMounts {} cstr cstr
    fun Cell* getCellAt {} float float
  }
}

cxx src/secondary/planet_generator.hxx
unsafe {
  const {planetgen::width  planetgen_width } unsigned
  const {planetgen::height planetgen_height} unsigned
  class final {planetgen::Parameters PlanetGeneratorParms} {} {
    constructor default
    var seed unsigned
    var continents unsigned
    var largeIslands unsigned
    var smallIslands unsigned
    var islandGrouping float
    var landSlope float
    var oceans unsigned
    var seas unsigned
    var lakes unsigned
    var rivers unsigned
    var mountainRanges unsigned
    var mountainSteepness float
    var enormousMountains unsigned
    var craters unsigned
    var maxCraterSize float
    var equatorTemperature float
    var solarEquator float
    var polarTemperature float
    var altitudeTemperatureDelta float
    var waterTemperatureDelta float
    var freezingPoint float
    var humidity float
    var vapourTransport float
    var mountainBlockage float
    var vegitationHumidity float
    var cities unsigned
    var maxCitySize float
    var cityGrouping float
    var waterColour unsigned
    var vegitationColour unsigned
    var lowerPlanetColour unsigned
    var upperPlanetColour unsigned
  }

  fun void {planetgen::begin planetgen_begin} planetgen::Parameters*
  fun cstr  {planetgen::what planetgen_what}
  fun float {planetgen::progress planetgen_progress}
  fun bool  {planetgen::done planetgen_done}
  fun void  {planetgen::kill planetgen_kill}
  fun void  {planetgen::save planetgen_save} cstr cstr
}

cxx src/camera/hud.hxx
fun void {hud_user_messages::setmsg set_hud_message} unsigned cstr

cxx src/secondary/global_chat.hxx
fun void {global_chat::post global_chat_post} cstr
fun void {global_chat::postLocal global_chat_post_local} cstr
fun void {global_chat::postRemote global_chat_post_remote} cstr

cxx src/globals.hxx src/core/game_state.hxx
unsafe {
  var state GameState* steal
}
const PLATFORM cstr
const screenW int
const screenH int
const vheight float
unsafe {
  var generalAlphaBlending bool
  var alphaBlendingEnabled bool
  var smoothScaling bool
  var highQuality bool
  var antialiasing bool
}
const headless bool
unsafe {
  var cameraX1 float
  var cameraX2 float
  var cameraY1 float
  var cameraY2 float
  var cameraCX float
  var cameraCY float
  var cameraZoom float
  var cursorX int
  var cursorY int
  var oldCursorX int
  var oldCursorY int
}
const currentFrameTime float
const currentFrameTimeLeft float
const currentVFrameLast bool
const frameRate unsigned
const sparkCountMultiplier float
const gameClock unsigned

cxx src/ship/cell/cell.hxx
const STD_CELL_SZ float

cxx src/tcl_iface/square_icon.hxx
unsafe {
  enum SquareIcon::LoadReq SILR {SquareIcon::Strict Strict} \
                                {SquareIcon::Lax    Lax   } \
                                {SquareIcon::Scale  Scale }
  class final SquareIcon {} {
    constructor default
    fun bool load {} cstr unsigned SquareIcon::LoadReq
    fun void unload
    fun bool isLoaded const
    fun void draw const
    fun bool save const cstr
  }
}

# Networking stuff
cxx src/net/antenna.hxx src/net/tuner.hxx src/net/globalid.hxx
unsafe {
  class final Tuner {}
  class final GlobalID {}
  class final Antenna {} {
    constructor default
    var tuner Tuner*
    fun void setInternetInformation4 {} Uint8 Uint8 Uint8 Uint8 Uint16
    fun void setInternetInformation6 {} \
      Uint16 Uint16 Uint16 Uint16 Uint16 Uint16 Uint16 Uint16 Uint16
    fun GlobalID* getGlobalID4
    fun GlobalID* getGlobalID6
    fun bool hasV4
    fun bool hasV6
    fun void processIncomming
  }
  const antenna Antenna
  var packetDropMask unsigned
}

cxx src/net/tuner.hxx
unsafe {
  class final Tuner {} {
    constructor default
  }
}

cxx src/net/globalid.hxx
unsafe {
  class final GlobalID {} {
    constructor default
    fun string toString
  }
}

cxx src/net/packet_processor.hxx
unsafe {
  class final PacketProcessor {} {
  }

  class final NetworkConnection PacketProcessor
}

cxx src/net/network_assembly.hxx src/sim/game_field.hxx src/net/antenna.hxx \
    src/net/tuner.hxx src/net/packet_processor.hxx \
    src/net/network_connection.hxx
unsafe {
  class final NetworkAssembly {} {
    const field GameField*
    const antenna Antenna*
    constructor default GameField* Antenna*
    fun Tuner* getTuner
    fun unsigned numConnections
    fun NetworkConnection* getConnection {} unsigned
    fun void addConnection {} {NetworkConnection* steal}
    fun void removeConnection {} unsigned
    fun void addPacketProcessor {} {PacketProcessor* steal}
    fun void update {} unsigned
    fun void setFieldSize {} float float
    fun void changeField {} GameField*
  }
}

cxx src/net/network_connection.hxx src/net/network_assembly.hxx \
    src/net/synchronous_control_geraet.hxx
unsafe {
  class final SynchronousControlGeraet {}
  enum NetworkConnection::Status {} \
    {NetworkConnection::Connecting} {NetworkConnection::Established} \
    {NetworkConnection::Ready} {NetworkConnection::Zombie}
  class final NetworkConnection PacketProcessor {
    const parent NetworkAssembly*
    const scg SynchronousControlGeraet*
    var blameMask unsigned
    fun void update {} unsigned
    fun NetworkConnection::Status getStatus
    fun string getDisconnectReason
    fun void setFieldSize {} float float
  }
}

cxx src/net/connection_listener.hxx
unsafe {
  class final ConnectionListener PacketProcessor {
  }
}

cxx src/net/network_geraet.hxx src/net/network_connection.hxx
unsafe {
  class final InputNetworkGeraet {} {
  }
  class final OutputNetworkGeraet {} {
  }

  newFunType {InputNetworkGeraet* steal} NetworkConnection*
}

cxx src/net/synchronous_control_geraet.hxx
unsafe {
  class final SynchronousControlGeraet {} {
    fun unsigned openChannel {} {OutputNetworkGeraet* steal} unsigned
  }
}

cxx src/net/game_advertiser.hxx src/net/tuner.hxx
unsafe {
  class final GameAdvertiser PacketProcessor {
    constructor default Tuner* bool unsigned unsigned bool cstr bool
    fun void setOverseerId {} unsigned
    fun void setPeerCount {} unsigned
    fun void setGameMode {} cstr
  }
}

cxx src/net/game_discoverer.hxx src/net/tuner.hxx src/net/antenna.hxx
unsafe {
  class final GameDiscoverer PacketProcessor {
    constructor default Tuner*
    fun void start
    fun void poll {} Antenna*
    fun float progress
  }
}

cxx src/net/async_ack_geraet.hxx
unsafe {
  class final AsyncAckGeraet {} {}
}

cxx src/net/seq_text_geraet.hxx src/net/async_ack_geraet.hxx
unsafe {
  class final SeqTextOutputGeraet OutputNetworkGeraet {
    constructor default AsyncAckGeraet*
    fun void send {} string
  }
  # While this is technically a ReliableSender, Tcl doesn't need
  # to know about that class.
  class abstract-extendable SeqTextInputGeraet InputNetworkGeraet {
    constructor default AsyncAckGeraet*
    fun void receiveText {noth purevirtual} string
  }
}

cxx src/net/network_game.hxx src/net/network_connection.hxx src/net/abuhops.hxx
unsafe {
  class final Peer {} {
    const gid GlobalID
    const nid unsigned
    const overseerReady bool
    const connectionAttempts unsigned
    const cxn NetworkConnection*
    const screenName string
    const receivedStx bool
  }
  class abstract-extendable NetIface {} {
    constructor default
    fun void addPeer purevirtual Peer*
    fun void delPeer purevirtual Peer*
    fun void setOverseer purevirtual Peer*
    fun void receiveBroadcast purevirtual Peer* cstr
    fun void receiveOverseer purevirtual Peer* cstr
    fun void receiveUnicast purevirtual Peer* cstr
    fun bool alterDatp purevirtual Peer* cstr
    fun bool alterDats purevirtual cstr
    fun void setGameMode purevirtual cstr
    fun cstr getGameMode purevirtual
    fun void connectionLost purevirtual cstr
    fun string getFullDats purevirtual
    fun void receiveShip purevirtual NetworkConnection* Ship*
  }
  class final NetworkGame {} {
    constructor default GameField*
    fun Peer* getLocalPeer
    fun Peer* getOverseer
    fun Peer* getPeerByConnection {} NetworkConnection*
    fun Peer* getPeerByNid {} unsigned
    fun string getDisconnectReason
    fun void setNetIface {} NetIface*
    fun void setAdvertising {} cstr
    fun void stopAdvertising
    fun void startDiscoveryScan
    fun float discoveryScanProgress
    fun bool discoveryScanDone
    fun string getDiscoveryResults
    fun void setLocalPeerName {} cstr
    fun void setLocalPeerNID {} unsigned
    fun void setLocalPeerNIDAuto {}
    fun void connectToNothing {} bool bool
    fun void connectToDiscovery {} unsigned
    fun void connectToLan {} cstr unsigned
    #fun void connectToInternet {} cstr
    #fun void startUdpHolePunch {} bool
    #fun bool hasInternet4
    #fun bool hasInternet6
    fun void update {} unsigned
    fun void updateFieldSize
    fun void changeField {} GameField*

    fun void alterDats {} string Peer*
    fun void alterDatp {} string Peer*
    fun void sendUnicast {} string Peer*
    fun void sendOverseer {} string Peer*
    fun void sendBroadcast {} string
    fun void sendGameMode {} Peer*
    fun void setBlameMask {} Peer* unsigned
  }

  fun void {abuhops::connect abuhops_connect} unsigned cstr unsigned cstr
}

cxx src/secondary/confreg.hxx
enum Setting::Type ST {Setting::TypeInt    Int   } {Setting::TypeInt64   Int64} \
                      {Setting::TypeFloat  Float } {Setting::TypeBoolean Bool} \
                      {Setting::TypeString String} \
                      {Setting::TypeArray  Array } {Setting::TypeList    List} \
                      {Setting::TypeGroup  Group }
class final ConfReg {} {
  unsafe {
    fun void open {} string string
    fun void openLazily {} string string
    fun void create {} string string
    fun void close {} string
    fun void closeAll
    fun void modify {} string
    fun void unmodify {} string
    fun void sync {} string
    fun void syncAll
    fun void revert {} string
    fun void revertAll
    fun void addToWhitelist {} string
    fun void removeFromWhitelist {} string
    fun void clearWhitelist
    fun void setWhitelistOnly {} bool
    fun void renameFile {} string string
  }
  fun bool exists {} string
  fun bool loaded {} string
  fun bool {getBool bool} {} string
  fun int {getInt int} {} string
  fun float {getFloat float} {} string
  fun string {getStr str} {} string
  fun void {setBool setb} {} string bool
  fun void {setInt seti} {} string int
  fun void {setFloat setf} {} string float
  fun void {setString sets} {} string string
  fun void add {} string string Setting::Type
  fun void {addBool addb} {} string string bool
  fun void {addInt addi} {} string string int
  fun void {addFloat addf} {} string string float
  fun void {addString adds} {} string string string
  fun void remove {} string
  fun void {pushBack append} {} string Setting::Type
  fun void {pushBackBool appendb} {} string bool
  fun void {pushBackInt appendi}  {} string int
  fun void {pushBackFloat appendf} {} string float
  fun void {pushBackString appends} {} string string
  fun void remix {} string unsigned
  fun string {getName name} {} string
  fun void copy {} string string
  fun Setting::Type getType {} string
  fun int {getLength length} {} string
  fun bool isGroup {} string
  fun bool isArray {} string
  fun bool isList  {} string
  fun bool isAggregate {} string
  fun bool isScalar {} string
  fun bool isNumber {} string
  fun int getSourceLine {} string
}

const {conf globalConf} ConfReg
fun void confcpy string string

cxx src/core/lxn.hxx
unsafe {
  fun void {l10n::acceptLanguage l10n_acceptLanguage} cstr
  fun bool {l10n::loadCatalogue  l10n_loadCatalogue} char cstr
  fun void {l10n::purgeCatalogue l10n_purgeCatalogue} char
}
fun cstr {l10n::lookup _} char cstr cstr

cxx src/secondary/namegen.hxx
fun string {namegenGet namegenAny}
fun string {namegenGet namegenGet} cstr

# Slave interpreter interface
cxx
unsafe {
  fun Tcl_Interp* newInterpreter bool Tcl_Interp*
  fun void delInterpreter Tcl_Interp*
}
fun void {interpSafeSource safe_source} {cstr {check ok=validateSafeSourcePath(val);}}

cxx src/tcl_iface/slave_thread.hxx
unsafe {
  fun void bkg_start
  fun void bkg_req cstr
  fun void bkg_ans cstr
  fun void {bkg_req bkg_req2} cstr cstr
  fun void {bkg_ans bkg_ans2} cstr cstr
  fun cstr bkg_rcv
  fun cstr bkg_get
  fun void bkg_wait
}

cxx src/net/crypto.hxx
unsafe {
  fun void crypto_init cstr
  fun cstr crypto_rand
  fun cstr crypto_powm cstr cstr
}

cxx src/secondary/validation.hxx
unsafe {
  fun void performValidation int int int int
  fun int getValidationResultA
  fun int getValidationResultB
}

cxx src/net/network_test.hxx
unsafe {
  class final NetworkTest TestState {
    constructor default
  }
}

cxx src/audio/ship_mixer.hxx
unsafe {
  fun void {audio::ShipMixer::init ship_mixer_init}
  fun void {audio::ShipMixer::end ship_mixer_end}
}

cxx src/control/joystick.hxx
enum joystick::AxisType {} \
    {joystick::Axis Axis} \
    {joystick::BallX BallX} \
    {joystick::BallY BallY}
enum joystick::ButtonType {} \
    {joystick::Button Button} \
    {joystick::HatUp HatUp} \
    {joystick::HatDown HatDown} \
    {joystick::HatLeft HatLeft} \
    {joystick::HatRight HatRight}
unsafe {
  fun unsigned {joystick::count joystick_count}
  fun cstr {joystick::name joystick_name} unsigned
  fun unsigned {joystick::axisCount joystick_axisCount} \
      unsigned joystick::AxisType
  fun unsigned {joystick::buttonCount joystick_buttonCount} \
      unsigned joystick::ButtonType
  # Tcl doesn't need access to the controls themselves.
}

cxx src/secondary/versus_match.hxx
class final VersusMatch {} {
  fun bool step noth
  fun float score {const noth}
}
fun {VersusMatch* steal} {VersusMatch::create VersusMatch_create} \
    string unsigned string unsigned

cxx src/secondary/frame_recorder.hxx
unsafe {
  fun void {frame_recorder::enable frame_recorder_enable}
  fun void {frame_recorder::setFrameRate frame_recorder_setFrameRate} float
}

cxx
unsafe {
  fun void debugTclExports
}
