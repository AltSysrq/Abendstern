/**
 * @file
 * @author Jason Lingle
 * @brief Implements src/secondary/frame_recorder.hxx
 */

/*
 * frame_recorder.cxx
 *
 *  Created on: 19.04.2011
 *      Author: jason
 */

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include <GL/gl.h>
#include <SDL.h>

#include "frame_recorder.hxx"
#include "src/globals.hxx"

#ifdef FRAME_RECORDER_ENABLED

using namespace std;

#define MS_PER_FRAME 33.3333f
#define TIME_BETWEEN_SYNC 4000

static bool running;
static unsigned run;
static unsigned frame;
static float timeUntilNextFrame;
static float timeUntilForceSync;
static char filename[128];

void frame_recorder::init() {
  running = false;
  run = 0;
}

void frame_recorder::begin() {
  ++run;
  frame = 0;
  timeUntilNextFrame = 0;
  timeUntilForceSync = TIME_BETWEEN_SYNC;
  running = true;
}

void frame_recorder::end() {
  running = false;
}

bool frame_recorder::on() {
  return running;
}

void frame_recorder::update(float et) {
  if (!running) return;
  timeUntilNextFrame -= et;

  while (timeUntilNextFrame < 0) {
    timeUntilNextFrame += MS_PER_FRAME;

    static Uint32* data=NULL;
    if (!data) {
      data = new Uint32[screenW*screenH];
    }

    //Take screenshot
    glReadPixels(0, 0, screenW, screenH, GL_RGBA, GL_UNSIGNED_BYTE, data);

    //Set filename
    sprintf(filename, "recorder/%02d-%08d.bmp", run, frame);

    /* Save image as BMP
     * 0000+02  Signature = "BM"
     * 0002+04  File size, = 0x44+width*height*4
     * 0006+04  Zero
     * 000A+04  Offset, = 0x44
     * 000E+04  Header size, =0x28 (BITMAPINFOHEADER)
     * 0012+04  Width
     * 0016+04  Height
     * 001A+02  Planes = 1
     * 001C+02  BPP, = 0x20
     * 001E+04  Compression, = 3
     * 0022+04  Image size, = 4*width*height
     * 0026+04  Horiz resolution, = 0x1000
     * 002A+04  Vert resolution, = 0x1000
     * 002E+04  Palette size, = 0
     * 0032+04  "Important colours", = 0
     * 0036+04  Red mask, = 0x000000FF
     * 003A+04  Grn mask, = 0x0000FF00
     * 003E+04  Blu mask, = 0x00FF0000
     * 0042+02  Gap
     * 0044+end Image
     */
    unsigned fileSize = 0x44+screenW*screenH*4;
    unsigned imageSize = 4*screenW*screenH;
    unsigned char header[] = {
      'B', 'M',
      (unsigned char)(fileSize >>  0),
      (unsigned char)(fileSize >>  8),
      (unsigned char)(fileSize >> 16),
      (unsigned char)(fileSize >> 24), //File size
      0, 0, 0, 0, //Zero
      0x44, 0, 0, 0, //Offset
      0x28, 0, 0, 0, //Header size / BITMAPINFOHEADER
      (unsigned char)(screenW >>  0),
      (unsigned char)(screenW >>  8),
      (unsigned char)(screenW >> 16),
      (unsigned char)(screenW >> 24), //Width
      (unsigned char)(screenH >>  0),
      (unsigned char)(screenH >>  8),
      (unsigned char)(screenH >> 16),
      (unsigned char)(screenH >> 24), //height
      0x01, 0x00, 0x20, 0x00, //Planes, BPP
      3, 0, 0, 0, //Compression
      (unsigned char)(imageSize >>  0),
      (unsigned char)(imageSize >>  8),
      (unsigned char)(imageSize >> 16),
      (unsigned char)(imageSize >> 24), //Image size
      0x00, 0x10, 0x00, 0x00, //Horiz res
      0x00, 0x10, 0x00, 0x00, //Vert res
      0, 0, 0, 0, //Palette size
      0, 0, 0, 0, //Important colours
      0xFF, 0x00, 0x00, 0x00, //Red mask
      0x00, 0xFF, 0x00, 0x00, //Grn mask
      0x00, 0x00, 0xFF, 0x00, //Blu mask
      0x00, 0x00, //Gap
    };

    ofstream out(filename);
    out.write((const char*)header, sizeof(header));
    out.write((const char*)data, imageSize);
    out.close();

    //Next frame
    ++frame;
  }

  timeUntilForceSync -= et;
  if (timeUntilForceSync < 0) {
    #ifndef WIN32
    cout << "Forcing sync... " << flush;
    system("sync");
    cout << "done." << endl;
    #endif
    timeUntilForceSync = TIME_BETWEEN_SYNC;
  }
}

#endif /* FRAME_RECORDER_ENABLED */
