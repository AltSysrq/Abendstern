/**
 * @file
 * @author Jason Lingle
 * @brief Implements src/secondary/frame_recorder.hxx
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
 * frame_recorder.cxx
 *
 *  Created on: 19.04.2011
 *      Author: jason
 */

#ifndef HAVE_DRACHEN_H
//Do-nothing implementation
#include "frame_recorder.hxx"
void frame_recorder::init() {}
void frame_recorder::begin() {}
void frame_recorder::end() {}
bool frame_recorder::on() { return false; }
void frame_recorder::update(float) {}
void frame_recorder::enable() {}
void frame_recorder::setFrameRate(float) {}
#else

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cerrno>

#include <GL/gl.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <drachen.h>

#include "frame_recorder.hxx"
#include "src/globals.hxx"

using namespace std;

#define TIME_BETWEEN_SYNC 4000
#define HEADER_SIZE 68

static bool running, enabled = false;
static unsigned run;
static unsigned frame;
static float msPerFrame = 33.3333f;
static float timeUntilNextFrame;
static float timeUntilForceSync;
static char filename[128];

static SDL_Thread* thread;
static SDL_sem* bufferWritable, * bufferReadable;
static drachen_encoder* encoder;
static unsigned char* buffer = NULL;
static uint32_t* xform = NULL;

static int threadBody(void*);

static const drachen_block_spec blockSpec[2] = {
  { HEADER_SIZE, HEADER_SIZE },
  { 0xFFFFFFFFu, 32 },
};

void frame_recorder::init() {
  running = false;
  run = 0;
}

void frame_recorder::begin() {
  if (!enabled) return;
  ++run;
  frame = 0;
  timeUntilNextFrame = 0;
  timeUntilForceSync = TIME_BETWEEN_SYNC;
  running = true;

  bufferWritable = SDL_CreateSemaphore(1);
  bufferReadable = SDL_CreateSemaphore(0);

  sprintf(filename, "recorder/%02d.drach", run);

  unsigned bufferSize = HEADER_SIZE + screenW*screenH*4;

  if (!buffer)
    buffer = new unsigned char[bufferSize];
  if (!xform) {
    xform = new uint32_t[bufferSize];
    drachen_make_image_xform_matrix(xform,
                                    HEADER_SIZE,
                                    screenW,
                                    screenH,
                                    4,
                                    64, 64);
  }

  FILE* file = fopen(filename, "wb");
  if (!file) {
    fprintf(stderr, "Could not open %s: %s\n",
            filename, strerror(errno));
    SDL_DestroySemaphore(bufferWritable);
    SDL_DestroySemaphore(bufferReadable);
    running = false;
    return;
  }

  encoder = drachen_create_encoder(file, bufferSize, xform);
  if (!encoder || drachen_error(encoder)) {
    fprintf(stderr, "Could not allocate encoder.\n");
    if (encoder) {
      fprintf(stderr, "Error returned: %s\n", drachen_get_error(encoder));
      drachen_free(encoder);
    }
    SDL_DestroySemaphore(bufferWritable);
    SDL_DestroySemaphore(bufferReadable);
    running = false;
    return;
  }

  drachen_set_block_size(encoder, blockSpec);

  thread = SDL_CreateThread(threadBody, NULL);
}

void frame_recorder::end() {
  running = false;
  //Mark the buffer as readable so that the thread can continue and notice
  //we're done
  SDL_SemPost(bufferReadable);
  SDL_WaitThread(thread, NULL);
  SDL_DestroySemaphore(bufferReadable);
  SDL_DestroySemaphore(bufferWritable);
  drachen_free(encoder);
}

bool frame_recorder::on() {
  return running;
}

void frame_recorder::update(float et) {
  if (!running) return;
  timeUntilNextFrame -= et;

  while (timeUntilNextFrame < 0) {
    timeUntilNextFrame += msPerFrame;

    //Take screenshot
    glReadPixels(0, 0, screenW, screenH, GL_RGBA, GL_UNSIGNED_BYTE,
                 buffer+HEADER_SIZE);

    //Wait for the buffer to be writable
    SDL_SemWait(bufferWritable);

    //Set filename
    sprintf(filename, "%02d-%08d.bmp", run, frame);

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
    unsigned char header[HEADER_SIZE] = {
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
    memcpy(buffer, header, sizeof(header));

    //Allow the background thread to write this frame
    SDL_SemPost(bufferReadable);

    //Next frame
    ++frame;
  }

  timeUntilForceSync -= et;
  if (timeUntilForceSync < 0) {
    #if 0 //No longer useful
    cout << "Forcing sync... " << flush;
    system("sync");
    cout << "done." << endl;
    #endif
    timeUntilForceSync = TIME_BETWEEN_SYNC;
  }
}

void frame_recorder::enable() { enabled = true; }
void frame_recorder::setFrameRate(float rate) {
  msPerFrame = 1000.0f/rate;
}

static int threadBody(void*) {
  while (running) {
    //Wait for data
    SDL_SemWait(bufferReadable);
    //Exit if requested
    if (!running) break;

    if (drachen_encode(encoder, buffer, filename)) {
      fprintf(stderr, "Error encoding frame %s: %s\n",
              filename, drachen_get_error(encoder));
    }

    SDL_SemPost(bufferWritable);
  }

  return 0;
}

#endif /* HAVE_DRACHEN_H */
