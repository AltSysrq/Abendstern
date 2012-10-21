/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains the core driver functions for Abendstern.
 */

#include <GL/gl.h>
#include <SDL.h>
//#include <SDL_opengl.h>
#include <algorithm>
#include <cassert>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>
#include <cerrno>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <typeinfo>
#include <vector>

#include <libconfig.h++>

//For mkdir on Unix
#ifndef WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
//And _mkdir on Windows
#else
#include <direct.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef WIN32
#include <malloc.h> //For Windows-specific function
#elif defined(DEBUG)
//So we can enable all FPEs in DEBUG mode
//Why the heck must I specify the entire path?
//G++ gives no error if I don't, but then can't find
//anything defined within
#include </usr/include/fenv.h>
#endif

#include <tcl.h>

//#define ENABLE_X11_VSYNC

#ifdef ENABLE_X11_VSYNC
//#define USE_MGL_NAMESPACE
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>
#endif /* ENABLE_X11_VSYNC */

#include "abendstern.hxx"
#include "globals.hxx"
#include "audio/audio.hxx"
#include "control/joystick.hxx"
#include "core/game_state.hxx"
#include "core/init_state.hxx"
#include "exit_conditions.hxx"
#include "graphics/gl32emu.hxx"
#include "graphics/matops.hxx"
#include "secondary/frame_recorder.hxx"
#include "net/antenna.hxx"
#include "tcl_iface/bridge.hxx"
#include "tcl_iface/slave_thread.hxx"

using namespace std;
using namespace libconfig;

static SDL_Surface* screen;

#define clock_t Uint32

//Privately used for local timekeeping
static clock_t lastFrameClock;

/* For calculating FPS */
static time_t fpsLastReport;
static unsigned fps;
/* For calculating average FPS over long period */
static unsigned fpsSum, fpsSampleCount;

AbendsternGLType recommendedGLType;
bool preliminaryRunMode = false;
static bool preliminaryRunModeAuto = false;

//The command-line arguments
static char**   cmdargv;
static unsigned cmdargc;

#define vclock SDL_GetTicks

#ifdef PROFILE
  static Uint32 updateTime, drawTime;

  float gp_total;
  map<const char*, float> gp_profileByFPS[1001];
  float gp_totalByFPS[1001];
  float gp_totalByFPS_curr;
  struct GraphicProfile {
    const char* name;
    float time;
    GraphicProfile(pair<const char*, float> d)
    : name(d.first), time(d.second) {}
    GraphicProfile(const GraphicProfile& o)
    : name(o.name), time(o.time) {}
    GraphicProfile& operator=(const GraphicProfile& o) {
      name=o.name;
      time=o.time;
      return *this;
    }
    bool operator<(const GraphicProfile& o) const {
      return time>o.time;
    }
  };
  void printProfileInfo(const char* title, map<const char*,float> gp_profile,
                        float total) {
    vector<GraphicProfile> data;
    for (map<const char*, float>::iterator it=gp_profile.begin();
         it != gp_profile.end(); ++it) {
      data.push_back(GraphicProfile(*it));
    }
    sort(data.begin(), data.end());
    cout << title << endl;
    cout << right << setw(12) << "Seconds " << " %   Function" << endl;
    for (unsigned i=0; i<data.size(); ++i) {
      cout.precision(3);
      cout << setw(11) << right << fixed << data[i].time;
      cout.precision(1);
      cout << ' ' << setw(4) << right << (data[i].time/total*100)
           << ' ' << data[i].name+4 /*skip gp__*/ << endl;
    }
  }
#endif

/**
 * Peforms pre-shutdown cleanup. This is run automatically when the
 * program exits, and so should not be called externally.
 */
void shutdown();

/** Called on abort().
 * This has its name because issues with Python were almost the
 * sole cause of its calling. We no longer use Python, but
 * this function is useful.
 */
void pyterminate() {
  //terminate() is^H^Hwas almost always called because of Python
  cerr << "ABEND" << endl;
  exit(EXIT_ABEND);
}

//Data for command-line arguments
static bool forceHeadless=false;
static bool forceFullscreen=false;
static bool forceWindowed=false;
static unsigned forceWidth=0, forceHeight=0;
static unsigned forceBits=0;
static unsigned cacheSizeMB=32;
static GarbageCollectionStrategy lcgcs = libconfig::GCS_LazyProgressive;

/** Sets the environment up for running. */
bool init();
/** The primary game loop. Handles updating, drawing, and input events.
 */
void run();

/** Analyses the GL version string to determine which build type of Abendstern
 * is appropriate for the graphics hardware.
 */
static AbendsternGLType analyseGLVersion(const char* vers) {
  if (!vers) {
    cerr << "Can't determine OpenGL version: NULL returned" << endl;
    return THIS_GL_TYPE;
  }
  //It is rather annoying that we can't get integer versions.
  //However, the standard essentially dictates that the string
  //has the following format:
  //[0-9]+\.[0-9]+(\.[0-9]+)?(\s.*)?
  //If the decimal is not at position 1, assume that the version is
  //greater than 3 (ie, more than one digit); similarly, if the character
  //two after the first decimal is a digit, assume a "minor version" of
  //99 (ie, sufficient).
  unsigned major, minor=0;
  if (vers[1] != '.') major = 99;
  else {
    major = vers[0]-'0';
    //Test one position after single-digit minor
    if (vers[3] >= '0' && vers[3] <= '9')
      minor = 99;
    else
      minor = vers[2]-'0';
  }

  if (major > 3 || (major == 3 && minor >= 2)) return AGLT32;
  //Theoreretically, we're 2.1, but OpenGL ES will return 2.0
  //for the same feature set (and vanilla OpenGL 2.0 should be
  //rare enough), so count any major 2 as 2.1
  else if (major == 2) return AGLT21;
  else return AGLT14;
}

#ifdef WIN32
/** Pointer to the log output handle, or NULL if none is present. */
static ofstream* logoutPtr = NULL;
#endif /* WIN32 */

/** The main function. It is not reentrant, so no other code should call it.
 *
 * @param argc  The number of arguments passed (the length of argv)
 * @param argv  The arguments passed to the program
 * @return The exit status of the program
 */
int main(int argc, char** argv) {
  cmdargv = argv;
  cmdargc = argc;

  //On Windows, HOME (er, USERPROFILE) is not an appropriate location.
  //Since we need to set HOME anyway, APPDATA is the propper location.
  #ifdef WIN32
  {
    char env[1024];
    sprintf(env, "HOME=%s", getenv("APPDATA"));
    putenv(env);
  }
  #endif
  //Create home directory if it does not exist
  {
    char home[1024];
    sprintf(home, "%s/.abendstern", getenv("HOME"));
    #ifdef WIN32
      struct _stat s;
      if (_stat(home, &s)) {
        //Create
        if (_mkdir(home)) {
          fprintf(stderr,
                  "FATAL: Could not create or access home directory: %s\n",
                  strerror(errno));
          exit(EXIT_THE_SKY_IS_FALLING);
        }
      }
    #else
      struct stat s;
      if (stat(home, &s)) {
        if (mkdir(home, 0755)) {
          fprintf(stderr,
                  "FATAL: Could not create or access home directory: %s\n",
                  strerror(errno));
          exit(EXIT_THE_SKY_IS_FALLING);
        }
      }
    #endif
  }
  #ifdef WIN32
  {
    char llout[1024];
    sprintf(llout, "%s\\.abendstern\\launchlog.txt", getenv("HOME"));
    ofstream out(llout, ios::ate|ios::app);
    out << argv[0] << endl;
  }
  #endif

  //Chdir to DATADIR if that is defined and the current directory does not
  //contain abendstern.default.rc
  #ifdef DATADIR
  {
    struct stat s;
    if (stat(DEFAULT_CONFIG_FILE, &s)) {
      if (chdir(DATADIR)) {
        fprintf(stderr,
                "FATAL: Could not chdir to " DATADIR ": %s\n",
                strerror(errno));
        exit(EXIT_PLATFORM_ERROR);
      }
    } else {
      printf("Note: Operating in current directory instead of " DATADIR "\n");
    }
  }
  #endif /* DATADIR */

  setlocale(LC_NUMERIC, "C");
  //On Windows, replace stdout and stderr with log.txt
  #ifdef WIN32
  char logoutName[1024];
  sprintf(logoutName, "%s\\.abendstern\\log.txt", getenv("HOME"));
  static ofstream logout(logoutName, ios::ate|ios::app);
  logoutPtr = &logout;
  if (logout) {
    cout.rdbuf(logout.rdbuf());
    cerr.rdbuf(logout.rdbuf());
    freopen(logoutName, "a", stdout);
    freopen(logoutName, "a", stderr);
  }
  #endif /* WIN32 */
  /* Parse command-line arguments */
  for (int i=1; i<argc; ++i) {
    if      (0==strcmp(argv[i], "-headless")
         ||  0==strcmp(argv[i], "-Headless")
         ||  0==strcmp(argv[i], "-H"))          forceHeadless=true;
    else if (0==strcmp(argv[i], "-fullscreen")
         ||  0==strcmp(argv[i], "-f"))          forceFullscreen=true;
    else if (0==strcmp(argv[i], "-windowed")
         ||  0==strcmp(argv[i], "-Windowed")
         ||  0==strcmp(argv[i], "-W"))          forceWindowed=true;
    else if (0==strcmp(argv[i], "-width")
         ||  0==strcmp(argv[i], "-w")) {
      if (i+1==argc) {
        cerr << "Missing argument after width" << endl;
        exit(-1);
      }
      forceWidth=atoi(argv[++i]);
    }
    else if (0==strcmp(argv[i], "-height")
         ||  0==strcmp(argv[i], "-h")) {
      if (i+1==argc) {
        cerr << "Missing argument after height" << endl;
        exit(-1);
      }
      forceHeight=atoi(argv[++i]);
    }
    else if (0==strcmp(argv[i], "-bits")
         ||  0==strcmp(argv[i], "-bpp")
         ||  0==strcmp(argv[i], "-b")) {
      if (i+1==argc) {
        cerr << "Missing argument after bits" << endl;
        exit(-1);
      }
      forceBits=atoi(argv[++i]);
    }
    else if (0 == strcmp(argv[i], "-cache")
         ||  0 == strcmp(argv[i], "-c")) {
      if (i+1 == argc) {
        cerr << "Missing argument after cache" << endl;
        exit(-1);
      }
      cacheSizeMB = atoi(argv[++i]);
    }
    else if (0 == strcmp(argv[i], "-Fast")
         ||  0 == strcmp(argv[i], "-fast")
         ||  0 == strcmp(argv[i], "-F")) {
      fastForward = true;
    }
    else if (0 == strcmp(argv[i], "-prelim")) {
      preliminaryRunMode = true;
    }
    else if (0 == strcmp(argv[i], "-prelimauto")) {
      preliminaryRunModeAuto = true;
    }
    else if (0 == strcmp(argv[i], "-lcgcs-immediate")) {
      lcgcs = libconfig::GCS_Immediate;
    }
    else if (0 == strcmp(argv[i], "-lcgcs-progressive")) {
      lcgcs = libconfig::GCS_Progressive;
    }
    else if (0 == strcmp(argv[i], "-lcgcs-lazy")) {
      lcgcs = libconfig::GCS_Lazy;
    }
    else if (0 == strcmp(argv[i], "-lcgcs-lazy-progressive")) {
      lcgcs = libconfig::GCS_LazyProgressive;
    }
    else if (0 == strcmp(argv[i], "-lcgcs-none")) {
      lcgcs = libconfig::GCS_None;
    }
    else {
      if (strcmp(argv[i], "-?")
      &&  strcmp(argv[i], "-help") && strcmp(argv[i], "--help"))
        cerr << "Unknown option: " << argv[i] << endl;
      cout << "Usage: " << argv[0] << " [options]" << endl;
      cout << "Options:" << endl;
      cout << "  -H[eadless]    Run in server mode\n"
              "  -f[ullscreen]  Run in full-screen mode\n"
              "  -W[indowed]    Run in windowed mode\n"
              "  -h[eight] int  Override window height in pixels\n"
              "  -w[idth] int   Override window width in pixels\n"
              "  -b[its] int    Override bits-per-pixel\n"
              "  -c[ache] int   Set number of MB of RAM to use for config cache\n"
              "  -F[ast]        Force elapsed time for each frame to 10 ms\n"
              "  -prelim        Start in preliminary configuration mode\n"
              "  -?, -help      Print this help message" << endl;
      exit(EXIT_SUCCESS);
    }
  }

  initBridgeTLS();
  //For some reason, if we try to create this /after/ a longjmp,
  //Tcl crashes
  Tcl_Obj* key=Tcl_NewStringObj("-errorinfo", -1);
  Tcl_IncrRefCount(key);
  PUSH_TCL_ERROR_HANDLER(tclErr);
  if (tclErr) {
    cerr << "ABEND: Tcl" << endl;
    Tcl_Obj* options=Tcl_GetReturnOptions(invokingInterpreter, TCL_ERROR);
    Tcl_Obj* stackTrace;
    Tcl_IncrRefCount(options);
    Tcl_DictObjGet(NULL, options, key, &stackTrace);
    Tcl_DecrRefCount(options);
    Tcl_DecrRefCount(key);
    Tcl_IncrRefCount(stackTrace);
    cerr << "Stack trace:\n" << Tcl_GetStringFromObj(stackTrace, NULL) << endl;
    Tcl_DecrRefCount(stackTrace);
    exit(EXIT_SCRIPTING_BUG);
  }
  if (!init()) return EXIT_PLATFORM_ERROR;
  if (!headless) {
    cout << "OpenGL version: " << (const char*)glGetString(GL_VERSION)
        << " by " << (const char*)glGetString(GL_VENDOR) << endl;
    recommendedGLType = analyseGLVersion((const char*)glGetString(GL_VERSION));
    #ifndef AB_OPENGL_14
    cout << "GLSL version: " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    #endif
  }
  atexit(shutdown);
  run();
  //Time to end the program normally.
  //Before we destroy the config system, move any prelim_assume_version_pre to
  //prelim_assume_version since everything seems to have worked with the
  //assumed settings.
  Setting& settings(conf["conf"]);
  if (settings.exists("prelim_assume_version_pre")) {
    if (settings.exists("prelim_assume_version"))
      settings.remove("prelim_assume_version");
    settings.add("prelim_assume_version", Setting::TypeString);
    settings["prelim_assume_version"] =
      (const char*)settings["prelim_assume_version_pre"];
    settings.remove("prelim_assume_version_pre");
    conf.modify("conf");
    conf.sync("conf");
  }
  #ifdef WIN32
  //Callbacks regestired by Tcl cause deadlock on termination in Windows
  //(I have no idea why, the debugger is useless in this context...).
  //Manually call shutdown(), then kill self on Windows
  shutdown();
  TerminateProcess(GetCurrentProcess(),0);
  #endif /* WIN32 */
  return EXIT_NORMAL;
}

bool init() {
  set_terminate(pyterminate);
  srand(time(NULL));
  /* From:
   * http://msdn.microsoft.com/en-us/library/a6x53890.aspx
   * "_set_sbh_threshold sets the current threshold value for the small-block
   * heap. The default threshold size is zero for Windows 2000 and later
   * operating systems. By default, the small-block heap is not used on Windows
   * 2000 and later operating systems, though _set_sbh_threshold can be called
   * with a nonzero value to enable the small-block heap in those instances."
   *
   * From what I've read, enabling this will improve performance drastically.
   * Nobody seems to know why it was disabled, nor what the potential side-
   * effects of using it are or what a reasonable value might be.
   *
   * Commented out -- For some reason, in MSVC++2010 with a VC2010 project
   * (earlier, I had been using a VC2008 project in 2010), this no longer
   * exists.
   */
  #ifdef WIN32
  //_set_sbh_threshold(256); //How reasonable is this value?
  #endif

  if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK)) {
    printf("Unable to initialize SDL: %s\n", SDL_GetError());
    return false;
  }

  joystick::init();

  libconfig::setMaxPriStorage(cacheSizeMB*1024*1024);
  libconfig::setGarbageCollectionStrategy(lcgcs);
  if (const char* err = libconfig::openSwapFile()) {
    printf("Unable to open swap file for libconfig: %s\n", err);
    //Expand RAM storage so this won't be an issue
    libconfig::setMaxPriStorage(0xFFFFFFFF);
  }

  int bits;
  bool fullScreen;
  char configFileLocation[1024];
  sprintf(configFileLocation, "%s/.abendstern/%s", getenv("HOME"), CONFIG_FILE);
  try {
    conf.open(configFileLocation, "conf");
  } catch (ConfigException& e) {
    try {
      //Try opening the older abendstern.rc located in the CWD
      conf.open(CONFIG_FILE, "conf");
    } catch(ConfigException& e) {
      try {
        conf.open(DEFAULT_CONFIG_FILE, "conf");
      } catch (ConfigException& e) {
        cerr << "Error reading configuration: " << e.what() << endl;
        return false;
      }
    }
    //Save in the new location
    conf.renameFile("conf", configFileLocation);
    conf.sync("conf");
  }
  try {
    conf.open("shaders/stacks.rc", "shaders");
  } catch (ConfigException& e) {
    cerr << "Error reading shader stack info: " << e.what() << endl;
    return false;
  }

  Setting& settings=conf["conf"];
  //Use prelim mode if specified, or if auto is specified and conf.skip_prelim
  //is either nonexistent or is false.
  if (preliminaryRunModeAuto &&
      (!conf.exists("skip_prelim") || !(bool)conf["skip_prelim"])) {
    preliminaryRunMode = true;
  }

  if (preliminaryRunMode) {
    //Force graphics config
    forceWidth = 640;
    forceHeight = 480;
    forceBits = 0; //Use native
    forceFullscreen = false;
    forceWindowed = true;
  } else {
    //If prelim_assume_version is set, rename it to _pre (removing _pre first if
    //that exists) so that the user gets prompted for configuration again if
    //Abendstern crashes.
    if (settings.exists("prelim_assume_version")) {
      if (settings.exists("prelim_assume_version_pre"))
        settings.remove("prelim_assume_version_pre");
      settings.add("prelim_assume_version_pre", Setting::TypeString);
      settings["prelim_assume_version_pre"] =
        (const char*)settings["prelim_assume_version"];
      settings.remove("prelim_assume_version");
      conf.modify("conf");
      conf.sync("conf");
    }
  }

  try {
    headless=forceHeadless || settings["display"]["headless"];
    //Can't use prelim mode when headless
    if (headless)
      preliminaryRunMode = false;

    if (!headless) {
      if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return false;
      }

      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
      SDL_WM_SetCaption("Abendstern", "Abendstern");
    }
    screenW=(forceWidth? forceWidth : settings["display"]["width"]);
    screenH=(forceHeight? forceHeight : settings["display"]["height"]);
    bits=(forceBits? forceBits : settings["display"]["bits"]);
    if (screenW == 0 || screenH == 0) {
      if (headless) {
        screenW=640;
        screenH=480;
        vheight = 480.0f/640.0f;
      } else {
        const SDL_VideoInfo* vi = SDL_GetVideoInfo();
        screenW = vi->current_w;
        screenH = vi->current_h;
        cout << screenW << 'x' << screenH << endl;
      }
    }
    generalAlphaBlending=settings["graphics"]["full_alpha_blending"];
    alphaBlendingEnabled=generalAlphaBlending || settings["graphics"]["any_alpha_blending"];
    smoothScaling=settings["graphics"]["smooth_scaling"];
    highQuality=settings["graphics"]["high_quality"];
    antialiasing=settings["graphics"]["antialiasing"];
    fullScreen=(forceFullscreen || (!forceWindowed && settings["display"]["fullscreen"]));
  } catch (ConfigException& e) {
    cerr << "Error with configuration: " << e.what() << endl;
    return false;
  }

  cout << "Abendstern Pre-Alpha" << endl;
  #if defined(DEBUG)
  cout << "Debug build" << endl;
  #elif defined(PROFILE)
  cout << "Profile build" << endl;
  #else
  cout << "Optimised build" << endl;
  #endif
  cout << "Build date: " __DATE__ " " __TIME__ << endl;

  if (!headless) {
    screen=SDL_SetVideoMode(screenW, screenH, bits, SDL_OPENGL | (fullScreen? SDL_FULLSCREEN : 0));
    #ifdef AB_OPENGL_14
    gl32emu::init();
    #endif
    mInit();
    if (!screen) {
      printf("Unable to initialize video: %s\n", SDL_GetError());
      return false;
    }
    if (fullScreen)
      //Under KDE4, we need to grab the mouse explicitly
      SDL_WM_GrabInput(SDL_GRAB_ON);

    #if defined(WIN32) && !defined(AB_OPENGL_14)
    glewInit();
    #endif /* WIN32 */

    glViewport(0, 0, screenW, screenH);

    if (alphaBlendingEnabled) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    /*
    if (antialiasing) {
      glEnable(GL_POINT_SMOOTH);
      glEnable(GL_LINE_SMOOTH);
    }
    */
    glDisable(GL_DITHER);
    glDisable(GL_DEPTH_TEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
//    glPixelStorei(GL_PACK_ALIGNMENT, 8);
  }

  fpsLastReport=time(NULL);
  fps=fpsSum=fpsSampleCount=0;

  #ifdef PROFILE
  for (int i=0; i<1001; ++i) gp_totalByFPS[i]=0;
  gp_total=0;
  gp_totalByFPS_curr=0;
  updateTime=drawTime=0;
  #endif

  state=new InitState();
  if (!headless) state->configureGL();

  frame_recorder::init();

  //Enable all floating-point exceptions on non-Windows debug
  #if defined(DEBUG) && !defined(WIN32)
  assert(-1 != feenableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID));
  #endif

  return true;
}

void run() {
  SDL_Event event;

  while (state) {
    libconfig::garbageCollection();

    time_t currentTime=time(NULL);

    //We don't want to update(0)
    while (vclock()==lastFrameClock && !fastForward) /* doNothing(); */;

    vclock_t elapsed;
    if (fastForward) {
      lastFrameClock = gameClock += 10;
      elapsed = 10;
    } else {
      vclock_t curr=vclock();
      elapsed=curr-lastFrameClock;
      lastFrameClock=curr;
      gameClock = SDL_GetTicks();
    }
    float elapsedMillis = elapsed*1000/(float)VCLOCKS_PER_SEC;

    #ifdef PROFILE
    Uint32 updateStart=gameClock;
    #endif
    GameState* ret=state->update(elapsedMillis < 100? elapsedMillis : 100);
    if (ret) {
      if (ret==state) ret=NULL;
      delete state;
      state=ret;
      if (state && !headless) state->configureGL();
    }
    #ifdef PROFILE
    updateTime += SDL_GetTicks()-updateStart;
    #endif
    if (!headless) {
      glClear(GL_COLOR_BUFFER_BIT);
      #ifdef PROFILE
      timeval drawStart;
      gettimeofday(&drawStart, NULL);
      Uint32 drawStartMS=SDL_GetTicks();
      #endif
      if (state) state->draw();
      #ifdef PROFILE
      timeval drawEnd;
      gettimeofday(&drawEnd, NULL);
      useconds_t drawTime=drawEnd.tv_usec-drawStart.tv_usec;
      drawTime %= 1000000;
      gp_total += drawTime/1000000.0f;
      gp_totalByFPS_curr += drawTime/1000000.0f;
      ::drawTime += SDL_GetTicks()-drawStartMS;
      #endif
      glFinish();
      #ifdef ENABLE_X11_VSYNC
      unsigned retraceCount;
      glXGetVideoSyncSGI(&retraceCount);
      glXWaitVideoSyncSGI(2, (retraceCount+1)&1, &retraceCount);
      #endif /* ENABLE_X11_VSYNC */
      vclock_t rbegin = vclock();
      frame_recorder::update(elapsedMillis);
      //Don't count time spent saving the image toward elapsed time
      vclock_t rend = vclock();
      lastFrameClock += (rend-rbegin);
      SDL_GL_SwapBuffers();
    }

    joystick::update();
    while (SDL_PollEvent(&event) && state) {
      switch(event.type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN:
          if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_PAUSE) {
            if (frame_recorder::on()) frame_recorder::end();
            else                      frame_recorder::begin();
          }
                                  state->keyboard(&event.key);   break;
        case SDL_MOUSEMOTION:     state->motion(&event.motion);  break;
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN: state->mouseButton(&event.button);   break;
        case SDL_QUIT: state=NULL; break;
      }
      foreignLoseControl(&event);
    }

    ++fps;
    currentTime=time(NULL);
    if (difftime(currentTime, fpsLastReport)>=1) {
      #ifdef PROFILE
      gp_totalByFPS[fps<=1000? fps:1000]+=gp_totalByFPS_curr;
      gp_totalByFPS_curr=0;
      for (map<const char*,float>::iterator it=gp_currFPSProfile.begin();
           it!=gp_currFPSProfile.end();
           ++it)
        gp_profileByFPS[fps<=1000? fps:1000][(*it).first]+=(*it).second;
      gp_currFPSProfile.clear();
      #endif //PROFILE
      frameRate = fps;
      cout << "FPS: " << fps << "; "
           << "config RAM: " << getCurrPriStorage()/1024 << '/' << getMaxPriStorage()/1024
           << " kB; config swap: " << getCurrSecStorage()/1024 << " kB" << endl;

      //Ignore menus for average
      //(Then again, now the menus run at 300 FPS and
      //the game at 500)
      //if (fps < 500) {
        fpsSum+=fps;
        ++fpsSampleCount;
      //}
      fps=0;
      fpsLastReport=currentTime;
    }
  }
}

void shutdown() {
  bkg_term();
  audio::term();
  joystick::close();
  SDL_Quit();
  libconfig::destroySwapFile();
  if (fpsSampleCount > 0) {
    cout << "Average FPS: " << fpsSum/(float)fpsSampleCount << endl;
    //Print graphic profiling info
    #ifdef PROFILE
    cout << "Time spent updating: " << updateTime << " ms ("
         << (int)(updateTime/(float)(updateTime+drawTime)*100.0f) << "%), "
         << "time spent drawing: " << drawTime << " ms ("
         << (int)(drawTime/(float)(updateTime+drawTime)*100.0f) << "%)"
         << endl;
    printProfileInfo("General graphics profile:", gp_profile, gp_total);
    char specTitle[128];
    for (int i=0; i<1000; ++i) if (gp_totalByFPS[i]>0) {
        sprintf(specTitle, "Graphics profile at framerate %d", i);
        printProfileInfo(specTitle, gp_profileByFPS[i], gp_totalByFPS[i]);
    }
    #endif
  }
}

void exitPreliminaryRunMode() {
  /* Close any sockets we have open so that the new process can use the same
   * port numbers.
   * (We don't rely on the port numbers being the same, but this greatly
   * reduces port number contention.)
   */
  antenna.close();
#ifdef WIN32
  const char* newexe;
  switch (recommendedGLType) {
  case AGLT14: newexe = "bin\\abw32gl14.exe"; break;
  case AGLT21: newexe = "bin\\abw32gl21.exe"; break;
  case AGLT32: newexe = "bin\\abw32gl32.exe"; break;
  }

  //Build a string command, excluding -prelim and -prelimauto
  ostringstream out(newexe);
  for (unsigned i = 1; i < cmdargc; ++i)
    if (0 != strcmp(cmdargv[i], "-prelim") &&
        0 != strcmp(cmdargv[i], "-prelimauto"))
      out << " " << cmdargv[i];

  //Need to close log outputs so the new process can open the files.
  if (logoutPtr)
    logoutPtr->close();
  fclose(stdout);
  fclose(stderr);
  shutdown();
  STARTUPINFOA sinfo = {
    sizeof(STARTUPINFO),
    0 //Init rest with zeros as well
  };
  PROCESS_INFORMATION info;
  CreateProcessA(newexe, (LPSTR)out.str().c_str(), NULL, NULL, FALSE,
                 CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &sinfo, &info);
  CloseHandle(info.hProcess);
  CloseHandle(info.hThread);
  exit(EXIT_NORMAL);
#else /* UNIX */
  //New command-line arguments.
  //Static so that the destructor won't be called
  static vector<char*> nargs;

  /* The final two characters of our command should be digits corresponding to
   * the OpenGL version. By changing these, we can easily locate the new
   * executable.
   */
  string progname(cmdargv[0]);
  if (progname.size() < 3 ||
      !isdigit(progname[progname.size()-2]) ||
      !isdigit(progname[progname.size()-1])) {
    cerr << "Cannot switch to new Abendstern process." << endl;
    cerr << "Executable name does not meet assumptions: " << progname << endl;
    exit(EXIT_THE_SKY_IS_FALLING);
  }

  //Alter the program name according to type
  char ca, cb;
  switch (recommendedGLType) {
  case AGLT14: ca = '1', cb = '4'; break;
  case AGLT21: ca = '2', cb = '1'; break;
  case AGLT32: ca = '3', cb = '2'; break;
  //Default should never be executed, but handle it so G++ can see there is no
  //code path where ca and cb are uninitialised.
  //(I don't understand why it considers invalid enumeration values legitimate
  // for the purpose of warnings...)
  default:
    cerr << "Unexpected recommendedGLType: " << recommendedGLType << endl;
    exit(EXIT_THE_SKY_IS_FALLING);
  }
  progname[progname.size()-2] = ca;
  progname[progname.size()-1] = cb;

  nargs.push_back(strdup(progname.c_str()));

  //Add other arguments which are not -prelim or -prelimauto
  for (unsigned i = 1; i < cmdargc; ++i)
    if (0 != strcmp(cmdargv[i], "-prelim") &&
        0 != strcmp(cmdargv[i], "-prelimauto"))
      nargs.push_back(cmdargv[i]);
  //Add terminating NULL
  nargs.push_back(NULL);

  //Release our resources
  shutdown();

  //Start the new process.
  //If all goes well, execv() will never return
  execv(nargs[0], &nargs[0]);

  //Something went wrong
  cerr << "Could not start new Abendstern process: " << strerror(errno) << endl;
  exit(EXIT_PLATFORM_ERROR);
#endif
}
