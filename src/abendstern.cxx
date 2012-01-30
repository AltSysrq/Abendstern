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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <typeinfo>
#include <vector>
#include <libconfig.h++>
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

#include "globals.hxx"
#include "core/game_state.hxx"
#include "core/init_state.hxx"
#include "graphics/matops.hxx"
#include "graphics/gl32emu.hxx"
#include "secondary/frame_recorder.hxx"
#include "audio/audio.hxx"
#include "tcl_iface/bridge.hxx"
#include "tcl_iface/slave_thread.hxx"
#include "exit_conditions.hxx"

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

/** Sets the environment up for running. */
bool init();
/** The primary game loop. Handles updating, drawing, and input events.
 */
void run();

enum AbendsternGLType { AGLT14, AGLT21, AGLT32 };
#if defined(AB_OPENGL_14)
#define THIS_GL_TYPE AGLT14
#elif defined(AB_OPENGL_21)
#define THIS_GL_TYPE AGLT21
#else
#define THIS_GL_TYPE AGLT32
#endif

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

/** The main function. It is not reentrant, so no other code should call it.
 *
 * @param argc  The number of arguments passed (the length of argv)
 * @param argv  The arguments passed to the program
 * @return The exit status of the program
 */
int main(int argc, char** argv) {
  setlocale(LC_NUMERIC, "C");
  //On Windows, replace stdout and stderr with log.txt
  #ifdef WIN32
  static ofstream logout("log.txt", ios::trunc);
  if (logout) {
    cout.rdbuf(logout.rdbuf());
    cerr.rdbuf(logout.rdbuf());
    freopen("log.txt", "a", stdout);
    freopen("log.txt", "a", stderr);
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
    else if (0 == strcmp(argv[i], "-!")) {
      //Flag does nothing (suppression occurs because switching is
      //disabled if ANY arguments were passed)
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
              "  -!             Don't switch to a better build based on GL version\n"
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
  cout << "OpenGL version: " << (const char*)glGetString(GL_VERSION)
       << " by " << (const char*)glGetString(GL_VENDOR) << endl;
  AbendsternGLType aglt = analyseGLVersion((const char*)glGetString(GL_VERSION));
  if (aglt != THIS_GL_TYPE) {
    cerr << "Warning: This is not the best build of Abendstern to use, given your "
            "OpenGL version." << endl;
    char* newexe = NULL;
    if (argc != 1) {
      cerr << "Cannot automatically switch due to command-line arguments." << endl;
      cerr << "Assuming you know what you are doing." << endl;
    #ifndef WIN32
    } else {
      cerr << "Automatic switching to appropriate type is only supported on Windows." << endl;
    }
    #else
    } else {
      switch (aglt) {
        case AGLT14: newexe = "bin\\abw32gl14.exe"; break;
        case AGLT21: newexe = "bin\\abw32gl21.exe"; break;
        case AGLT32: newexe = "bin\\abw32gl32.exe"; break;
      }
    }
    #endif
    if (newexe) {
      cerr << "Automatically executing " << newexe << " instead." << endl;
      #ifdef WIN32
      //Need to close log outputs so the new process can open the files.
      logout.close();
      fclose(stdout);
      fclose(stderr);
      shutdown();
      STARTUPINFOA sinfo = {
        sizeof(STARTUPINFO),
        0 //Init rest with zeros as well
      };
      PROCESS_INFORMATION info;
      CreateProcessA(newexe, newexe, NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &sinfo, &info);
      CloseHandle(info.hProcess);
      CloseHandle(info.hThread);
      #endif /* WIN32 */
      return 0;
    }
  }
  #ifndef AB_OPENGL_14
  cout << "GLSL version: " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
  #endif
  atexit(shutdown);
  run();
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

  libconfig::setMaxPriStorage(cacheSizeMB*1024*1024);
  if (const char* err = libconfig::openSwapFile()) {
    printf("Unable to open swap file for libconfig: %s\n", err);
    //Expand RAM storage so this won't be an issue
    libconfig::setMaxPriStorage(0xFFFFFFFF);
  }

  int bits;
  bool fullScreen;
  try {
    conf.open(CONFIG_FILE, "conf");
  } catch (ConfigException& e) {
    try {
      conf.open(DEFAULT_CONFIG_FILE, "conf");
      conf.renameFile("conf", CONFIG_FILE);
    } catch (ConfigException& e) {
      cerr << "Error reading configuration: " << e.what() << endl;
      return false;
    }
  }
  try {
    conf.open("shaders/stacks.rc", "shaders");
  } catch (ConfigException& e) {
    cerr << "Error reading shader stack info: " << e.what() << endl;
    return false;
  }
  Setting& settings=conf["conf"];
  try {
    headless=forceHeadless || settings["display"]["headless"];
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
  //state=new ShaderTest();
  if (!headless) state->configureGL();

  #ifdef FRAME_RECORDER_ENABLED
  frame_recorder::init();
  #endif

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
    GameState* ret=state->update(elapsedMillis);
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
      #ifdef FRAME_RECORDER_ENABLED
      vclock_t rbegin = vclock();
      frame_recorder::update(elapsedMillis);
      //Don't count time spent saving the image toward elapsed time
      vclock_t rend = vclock();
      lastFrameClock += (rend-rbegin);
      #endif
      SDL_GL_SwapBuffers();
    }

    while (SDL_PollEvent(&event) && state) {
      switch(event.type) {
        case SDL_KEYUP:
        case SDL_KEYDOWN:
          #ifdef FRAME_RECORDER_ENABLED
          if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_PAUSE) {
            if (frame_recorder::on()) frame_recorder::end();
            else                      frame_recorder::begin();
          }
          #endif /* FRAME_RECORDER_ENABLED */
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
  SDL_Quit();
  libconfig::destroySwapFile();
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
