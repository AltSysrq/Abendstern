/**
 * @file
 * @author Jason Lingle
 *
 * @brief Provides the instances for declarations in src/globals.hxx.
 */

#include "globals.hxx"
GameState* state;
unsigned screenW, screenH;
bool headless, generalAlphaBlending, alphaBlendingEnabled,
     smoothScaling, highQuality, antialiasing;
float vheight;
unsigned long gameClock=0;
bool debug_freeze=false;
float cameraX1=0, cameraX2=999, cameraY1=0, cameraY2=999;
float cameraCX=5, cameraCY=5;
float cameraZoom=1;
int cursorX, cursorY, oldCursorX, oldCursorY;
std::pair<float,float> screenCorners[4];
float currentFrameTime, currentFrameTimeLeft;
bool currentVFrameLast=false;
bool fastForward;
ConfReg conf;
float sparkCountMultiplier=1.0f;
unsigned frameRate;
bool suppressRemoteHostTimeout = false;

#ifdef PROFILE
  std::map<const char*,float> gp_profile, gp_currFPSProfile;
#endif
