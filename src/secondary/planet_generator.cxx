/**
 * @file
 * @author Jason Lingle
 * @brief Implemntation of src/secondary/planet_generator.hxx
 */

/*
 * planet_generator.cxx
 *
 *  Created on: 12.03.2011
 *      Author: jason
 */

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>
#include <vector>
#include <set>

#include <libconfig.h++>
#include <png++/png.hpp>
#include <SDL.h>
#include <SDL_thread.h>

#include "planet_generator.hxx"
#include "src/globals.hxx"
#include "src/opto_flags.hxx"

using namespace libconfig;
using namespace std;

namespace planetgen {
  /* Information passed between the main and planet generation
   * threads. The semaphore should be locked before accessing or
   * modifying any of these data, except for the booleans, which
   * are intrinsically atomic.
   */
  static SDL_sem* semaphore=NULL;
  static volatile bool isGeneratorRunning=false;
  static float generatorProgress;
  static const char* generatorDescription;
  static SDL_Thread* thread;

  /* Results stored here when the generator completes. */
  static png::image<png::rgb_pixel>* resultDayImage=NULL, * resultNightImage=NULL;

  /* The function that does the actual generation.
   * Argument is a const Parameters*.
   */
  int generate_impl(void*);

  void begin(const Parameters& parms) {
    semaphore=SDL_CreateSemaphore(1);
    isGeneratorRunning=true;
    generatorDescription="Initialising";
    generatorProgress=0;
    thread=SDL_CreateThread(generate_impl, const_cast<Parameters*>(&parms));
  }

  const char* what() {
    if (SDL_SemWait(semaphore)) return SDL_GetError();
    const char* ret=generatorDescription;
    SDL_SemPost(semaphore);
    return ret;
  }

  float progress() {
    if (SDL_SemWait(semaphore)) return 0;
    float ret=generatorProgress;
    SDL_SemPost(semaphore);
    return ret;
  }

  bool done() {
    return !isGeneratorRunning;
  }

  void kill() {
    isGeneratorRunning=false;
    SDL_WaitThread(thread, NULL);
    if (resultDayImage) delete resultDayImage;
    if (resultNightImage) delete resultNightImage;
    SDL_DestroySemaphore(semaphore);
    semaphore=NULL;
  }

  void save(const char* dayfile, const char* nightfile) {
    resultDayImage->write(dayfile);
    delete resultDayImage;
    resultDayImage=NULL;
    resultNightImage->write(nightfile);
    delete resultNightImage;
    resultNightImage=NULL;
    kill();
  }

  /* Implement our own Mersenne Twister, to guarantee the exact
   * same sequence regardless of platform.
   */
  static Uint32 mt[624];
  static unsigned mtIndex;
  //Make sure it is 32-bit signed-safe, so the code can work correctly
  //when we translate to an unsigned-deficient language (C#)
  #define MTRAND_MAX 0x7FFFFFFF
  static void mtsrand(Uint32 seed) {
    mt[0] = seed;
    for (unsigned i=1; i<lenof(mt); ++i) {
      mt[i] = i + 1812433253 * (mt[i-1] ^ ((mt[i-1] >> 30) & 0x7FFFFFFF));
    }

    mtIndex=0;
  }

  static void mtpregen() {
    for (unsigned i=0; i<lenof(mt); ++i) {
      Uint32 y = (mt[i] & 0x80000000) | (mt[(i+1)%624] & 0x7FFFFFFF);
      mt[i] = mt[(i+397)%lenof(mt)] ^ (y >> 1);
      if (y&1)
        mt[i] = mt[i] ^ 0x9908B0DF;
    }
  }

  static Uint32 mtrand() {
    if (!mtIndex) mtpregen();
    Uint32 y = mt[mtIndex++];
    y ^= ((y >> 11) & 0x7FFFFFFF);
    y ^= ((y <<  7) & 0x9D2C5680);
    y ^= ((y << 15) & 0xEFC60000);
    y ^= ((y >> 18) & 0x7FFFFFFF);
    mtIndex %= lenof(mt);
    return y & MTRAND_MAX;
  }

  //planetify not yet implemented

  template<typename T>
  static inline T& coord(unsigned short x, unsigned short y, T* t) {
    return t[y*width + x];
  }

  static inline void setProgress(const char* desc, float prog) {
    cout << desc << ": " << prog*100 << "%" << endl;
    if (!SDL_SemTryWait(semaphore)) {
      generatorProgress=prog;
      generatorDescription=desc;
      SDL_SemPost(semaphore);
    }
    //Do nothing if we couldn't get the lock
  }

  /* I don't understand why inner structs can't be used
   * with templates...
   */
  struct seed {
    unsigned char writeDat;
    signed short x, y;
    unsigned char modulo;
  };
  struct elev_temp {
    unsigned char elev;
    unsigned short temp;
  };
  struct elev_temp_hum {
    unsigned char elev;
    unsigned short temp;
    float hum;
  };

  /* The generation algorithm works in the following passes:
   * + Seeding and growing. Plant the various land and water
   *   seeds, and grow until no seeds remain.
   * + Base altitude calculation. For each land point, determine
   *   altitude above sea-level based on distance from water and
   *   the land slope. Minimum 1, maximum 127 (both inclusive).
   * + Mountain ranges. Pick random pairs of points, and random
   *   numbers of mountains, and create mountains along that
   *   line.
   * + Enormous mountains. Pick random land points, and create
   *   enormous mountains there.
   * + Craters. Pick random land points, and create a hollow
   *   around the centre and a rim of increased altitude around
   *   that, possibly creating more water pixels.
   * + Rivers. Pick a random point. If it is not land, skip this
   *   river. Otherwise, place water going downhill until we can
   *   go down no longer.
   * + Temperature. Determine the temperature of every pixel.
   * + Humidity. Determine the humidity of every land pixel.
   * + Rasterisation. Convert the data to the day and night
   *   images.
   */
  int generate_impl(void* parmptr) {
    Parameters parm(*(const Parameters*)parmptr);

    mtsrand(parm.seed);

    /* The output from the seeding and growing pass is a
     * simple array of bytes, set to 1 for land and 2 for
     * water (0 is used internally for void).
     */
    unsigned char* data0;
    {
      unsigned pixelsLeft=width*height;
      data0 = new unsigned char[width*height];
      memset(data0, 0, width*height);

      vector<seed> seeds;

      //Plant seeds
      #define PLANT_SEED(d,m) { \
        seed s = {d, 0, 0, m}; \
        do { \
          s.x = mtrand()%width; \
          s.y = mtrand()%height; \
        } while (coord(s.x, s.y, data0)); \
        coord(s.x, s.y, data0)=d; \
        seeds.push_back(s); \
        --pixelsLeft; \
      }
      for (unsigned i=0; i<parm.continents; ++i)
        PLANT_SEED(1, 1)
      for (unsigned i=0; i<parm.largeIslands; ++i)
        PLANT_SEED(1, 2)
      //Planting smaller islands requires separate code
      {
        vector<seed> smallIslands;
        for (unsigned i=0; i<parm.smallIslands; ++i) {
          seed s = {1, 0, 0, 8};
          bool acceptByDist;
          do {
            s.x = mtrand()%width;
            s.y = mtrand()%height;
            //Find the distance to the nearest other island
            float minDist=10;
            for (unsigned j=0; j<smallIslands.size(); ++j) {
              float dx = smallIslands[j].x-s.x;
              if (fabs(dx) > width/2) dx = width-fabs(dx);
              float dy = smallIslands[j].y-s.y;
              float dist = sqrt(dx*dx + dy*dy)/(float)height;
              if (dist < minDist) minDist=dist;
            }

            acceptByDist = (minDist == 10
                         || mtrand()/(float)MTRAND_MAX < exp(-minDist*parm.islandGrouping));
          } while (coord(s.x, s.y, data0) || !acceptByDist);
          seeds.push_back(s);
          smallIslands.push_back(s);
          coord(s.x, s.y, data0) = 1;
          --pixelsLeft;
        }
      }
      for (unsigned i=0; i<parm.oceans; ++i)
        PLANT_SEED(2, 1)
      for (unsigned i=0; i<parm.seas; ++i)
        PLANT_SEED(2, 2)
      for (unsigned i=0; i<parm.lakes; ++i)
        PLANT_SEED(2, 8)

      unsigned char passNum=0;
      //Make passes until no seeds remain
      while (!seeds.empty() && isGeneratorRunning) {
        ++passNum;
        setProgress("Generating landforms", 1.0f - pixelsLeft/(float)width/(float)height);

        /* Cache the size when we start, so that we do not continuously
         * update new seeds added to the end.
         */
        unsigned size=seeds.size();
        for (unsigned i=0; i<size; ++i) {
          if (passNum % seeds[i].modulo) continue;
          //Choose random offsets to propagate the seed
          signed short ox = ((signed short)(mtrand()%3))-1;
          signed short oy = ((signed short)(mtrand()%3))-1;
          if (mtrand()&1)
            ox=0;
          else
            oy=0;
          signed short nx = (seeds[i].x+width+ox)%width;
          signed short ny = seeds[i].y+oy;
          if (ny < 0) ny=0;
          else if (ny >= height) ny=height-1;
          if (coord(nx, ny, data0)) {
            //Taken
            //Check whether the seed is actually alive
            bool dead=true;
            for (ox=-1; ox<=+1 && dead; ++ox)
              for (oy=seeds[i].y? -1 : 0; oy<=(seeds[i].y+1 < height? +1 : 0); ++oy)//Don't check dead here, that would have an extra penalty
                dead = dead && coord((seeds[i].x+ox+width)%width, seeds[i].y+oy, data0);
            if (dead) {
              //Seed is dead, move tail seed to this spot
              if (seeds.size() > 1)
                seeds[i] = seeds.back();
              seeds.pop_back();
              --i;
              --size;
            }
          } else {
            //Success
            --pixelsLeft;
            coord(nx, ny, data0) = seeds[i].writeDat;
            seed s = seeds[i];
            s.x=nx;
            s.y=ny;
            if ((mtrand() % s.modulo) == 0) s.modulo++;
            if (!s.modulo) s.modulo=0xFF;
            if ((mtrand() % s.modulo) == 0) s.modulo--;
            if (!s.modulo) s.modulo=1;
            seeds.push_back(s);
          }
        }
      }

      if (!isGeneratorRunning) {
        //Abort
        delete[] data0;
        return 0;
      }
    } //End seeding pass

    unsigned char* data1;

    /* Use exactly 256 random samples in the area surrounding each
     * land point to determine its altutide.
     * To improve speed, only calculate explicitly for 1 out of 16 pixels,
     * then bileniarly interpolate the rest.
     */
    {
      const signed short maxrad = (signed short)(height/parm.landSlope);
      data1 = new unsigned char[width*height];

      for (signed y=0; y<height && isGeneratorRunning; y += 4) {
        setProgress("Calculating altitude", y/(float)height*0.9f);
        for (signed x=0; x<width; x+=4) {
          if (coord(x, y, data0) == 2) {
            //Water
            coord(x, y, data1) = 0;
          } else {
            //Land
            float minDistSq=1.0e32f;
            signed minX = x - maxrad, maxX = x+maxrad, minY=y-maxrad, maxY=y+maxrad;
            if (minX < -width) {
              minX = 0;
              maxX = width-1;
            } else {
              if (minX < 0) {
                minX += width;
                maxX += width;
              }
              //Let maxX be greater than width, we'll fix that after choosing samples
            }
            if (minY < 0) minY=0;
            if (maxY >= height) maxY=height-1;

            for (unsigned i=0; i<256; ++i) {
              signed short rx = mtrand()%(maxX-minX)+minX;
              signed short ry = mtrand()%(maxY-minY)+minY;
              rx %= width;

              if (coord(rx, ry, data0) == 2) {
                float dx = rx-x, dy = ry-y;
                //Wrap-around
                if (fabs(dx) > width/2) dx = width-fabs(dx);
                float dsq = dx*dx + dy*dy;
                if (dsq < minDistSq) minDistSq=dsq;
              }
            }

            float altitude = sqrt(minDistSq)/height*parm.landSlope*127;
            coord(x, y, data1) = (unsigned char)(max(1.0f, min(127.0f, altitude)));
          }
        }
      }

      /* Interpolate the altitudes for entire rows */
      for (signed y=0; y<height && isGeneratorRunning; y+=4) {
        setProgress("Calculating altitude", 0.90f + y/(float)height*0.05f);
        for (signed x0=0; x0<width; x0+=4) {
          signed x1=x0+4;
          if (x1 >= width) x1=x0;
          float x0e = coord(x0, y, data1);
          float x1e = coord(x1, y, data1);
          for (signed xo=1; xo<4 && x0+xo<width; ++xo) {
            unsigned char type=coord(x0+xo, y, data0);
            if (type == 1)
              coord(x0+xo, y, data1) = (unsigned char)max(1.0f, min(127.0f,x0e*(4.0f-xo)/4.0f+x1e*(xo/4.0f)));
            else
              coord(x0+xo, y, data1) = 0;
          }
        }
      }

      /* Interpolate between rows */
      for (signed y0=0; y0<height && isGeneratorRunning; y0+=4) {
        setProgress("Calculating altitude", 0.95f + y0/(float)height*0.05f);
        signed y1=y0+4;
        if (y1 >= height) y1=y0;
        for (signed x=0; x<width; ++x) {
          float y0e = coord(x, y0, data1);
          float y1e = coord(x, y1, data1);
          for (signed yo=1; yo<4 && y0+yo<height; ++yo) {
            unsigned char type=coord(x, y0+yo, data0);
            if (type == 1)
              coord(x, y0+yo, data1) = (unsigned char)max(1.0f, min(127.0f,y0e*(4.0f-yo)/4.0f+y1e*(yo/4.0f)));
            else
              coord(x, y0+yo, data1) = 0;
          }
        }
      }

      delete[] data0;
      if (!isGeneratorRunning) {
        delete[] data1;
        return 0;
      }
    }

    /* Create mountain ranges. For each range, pick two points at
     * random. Try again, up to 1024 times, until both points are
     * on land (after 1024 times, go to the next range). Then, pick
     * a random number between 512 (inclusive) and 1536 (exclusive).
     * This will be the number of mountains. Mountain range clustering
     * is determined with a random integer between 2 (inclusive) and
     * 10 (exclusive).
     * For each mountain, a random point on the line is chosen, represented
     * as 0..1. The probability of keeping this point is 1-4*(point-0.5)^2.
     * As many tries as necessary are made to pass this test. Then, a random
     * distance between -1 and +1 (inclusive) is picked. This distance is
     * accepted on a probability of e^(-mountainClustering*abs(dist)). As
     * many tries as necessary are used to pass this test.
     * The distance along the line and the distance from the line are used to
     * calculate the centre of the new mountain. The peak height of the mountain
     * is determined with 255*rand^3*0.8, where rand is a single randomly-chosen
     * value between 0.2 and 1.0, inclusive. All non-water pixels within the
     * mountain's slope are set to the maximum of their current altitude and
     * the altitude required by the new mountain, using a slope of 4*landSlope.
     * This is an in-place transformation of data1.
     */
    {
      for (unsigned mountainRange=0; mountainRange<parm.mountainRanges && isGeneratorRunning; ++mountainRange) {
        setProgress("Creating mountain ranges", mountainRange/(float)parm.mountainRanges);

        signed p1x, p1y, p2x, p2y;
        unsigned triesLeft=1024;
        do {
          p1x=mtrand()%width;
          p1y=mtrand()%height;
          p2x=mtrand()%width;
          p2y=mtrand()%height;
        } while (!coord(p1x, p1y, data1) && !coord(p2x, p2y, data1) && --triesLeft);
        if (!triesLeft) continue; //give up

        unsigned numMountains = 512 + (mtrand()&1023);
        float mountainClustering = 2 + (mtrand()&7);
        for (unsigned mountain = 0; mountain < numMountains && isGeneratorRunning; ++mountain) {
          float lineOff, lineDist;
          do {
            lineOff = mtrand()/(float)MTRAND_MAX;
          } while (mtrand()/(float)MTRAND_MAX > 1.0f-4.0f*(lineOff-0.5f)*(lineOff-0.5f));
          do {
            lineDist = mtrand()/(float)MTRAND_MAX*2.0f-1.0f;
          } while (mtrand()/(float)MTRAND_MAX > exp(-mountainClustering*fabs(lineDist)));

          float tmprand=mtrand()/(float)MTRAND_MAX*0.8f+0.2f;
          float maxHeight = 255*tmprand*tmprand*tmprand*0.8f;

          //Find the centre point
          signed cx = (signed)(p1x + lineOff*(p2x-p1x) + (p2y-p1y)*lineDist);
          signed cy = (signed)(p1y + lineOff*(p2y-p1y) + (p2x-p1x)*lineDist);

          signed maxrad = (signed)(maxHeight/255.0f/parm.mountainSteepness/parm.landSlope*height);
          signed minx = cx-maxrad, maxx = cx+maxrad, miny = cy-maxrad, maxy = cy+maxrad;
          if (minx < 0) {
            minx += width;
            maxx += width;
          }
          //Allow maxx to be > width, we'll handle that later
          if (miny < 0) miny=0;
          if (maxy > height) maxy=height;

          for (signed y=miny; y<maxy; ++y) {
            for (signed xx=minx; xx<maxx; ++xx) {
              signed x=xx%width;
              if (!coord(x,y,data1)) continue;
              float curr=coord(x,y,data1);
              float dx = xx-cx;
              float dy = y-cy;
              float dist = sqrt(dx*dx + dy*dy)/height;
              float h = maxHeight - dist*parm.mountainSteepness*parm.landSlope*255;
              coord(x,y,data1) = max(curr, h);
            }
          }
        }
      }

      if (!isGeneratorRunning) {
        delete[] data1;
        return 0;
      }
    }

    /* Create enormous mountains. For each mountain, pick a random point.
     * If it is not land, try again, up to 1024 times, after which we give
     * up and move to the next mountain. Then, use the same algorithm to
     * raise surrounding land surfaces, but also apply the transformation
     * to oceans.
     * This is an in-place modification of data1.
     */
    {
      float slope = parm.landSlope*2;
      signed maxrad = (signed)(height/slope);

      for (unsigned mountain=0; mountain < parm.enormousMountains && isGeneratorRunning; ++mountain) {
        setProgress("Generating enormous mountains", mountain/(float)parm.enormousMountains);
        signed cx, cy;
        unsigned triesLeft=1024;
        do {
          cx = mtrand()%width;
          cy = mtrand()%height;
        } while (!coord(cx,cy,data1) && --triesLeft);
        if (!triesLeft) continue;

        signed minx = cx-maxrad, maxx = cx+maxrad, miny = cy-maxrad, maxy = cy+maxrad;
        if (minx < 0) {
          minx += width;
          maxx += width;
        }
        if (miny < 0) miny=0;
        if (maxy > height) maxy=height;

        for (signed y=miny; y<maxy; ++y) for (signed xx=minx; xx<maxx; ++xx) {
          signed x = xx%width;
          float dx = cx-xx;
          float dy = cy-y;
          float dist = sqrt(dx*dx + dy*dy);
          float curr = coord(x,y,data1);
          float h = 255-dist*slope/height*255;
          coord(x,y,data1) = max(curr,h);
        }
      }

      if (!isGeneratorRunning) {
        delete[] data1;
        return 0;
      }
    }

    /* Create craters. For each crater, select a random point. If it is not
     * land, try again, up to 1024 times, after which we give up and move on
     * to the next crater. Pick a random radius between maxCraterSize and
     * maxCraterSize/2 with the formula maxCraterSize*(0.5 + 0.5*rand^2), where
     * rand is a random number between 0 and 1 inclusive. The change in altitude
     * of the centre is -landSlope*2*radius. The change of any other point within
     * the radius is given by -landSlope*2*(radius-distance^2).
     * Then, pick a random integer between 8 and 19, inclusive. Pick a random angle,
     * between 0 and 2*pi, for each of these. Use these angles to determine that many
     * points around the centre of the crater, with radius radius*1.1. For all points
     * between radius and radius*1.2, adjust their altitude according to
     *   +max(0, 8*radius*(pi/4-angularDistanceFromNearestPoint)*(1-10*abs(pointRadius/radius-1.1)))
     * This is an in-place transformation of data1.
     */
    {
      for (unsigned crater=0; crater < parm.craters && isGeneratorRunning; ++crater) {
        setProgress("Generating craters", crater/(float)parm.craters);

        signed cx, cy;
        unsigned triesLeft=1024;
        do {
          cx = mtrand()%width;
          cy = mtrand()%height;
        } while (!coord(cx, cy, data1) && --triesLeft);
        if (!triesLeft) continue;

        float r = mtrand()/(float)MTRAND_MAX;
        float radius = parm.maxCraterSize*(0.5f + 0.5f*r*r)*height;
        signed maxrad = (signed)(1.2f*radius);
        signed minx = cx-maxrad, maxx = cx+maxrad, miny = cy-maxrad, maxy = cx+maxrad;
        if (minx < 0) {
          minx += width;
          maxx += width;
          cx += width;
        }
        if (miny < 0) miny=0;
        if (maxy > height) maxy=height;
        unsigned numRidges = 8 + (mtrand()&15);
        float ridges[20];
        for (unsigned i=0; i<numRidges; ++i)
          ridges[i] = mtrand()/(float)MTRAND_MAX*2.0f*pi;
        for (signed y = miny; y < maxy; ++y) for (signed xx = minx; xx < maxx; ++xx) {
          signed x = xx%width;
          float dx = cx-xx, dy = cy-y;
          float distSq = dx*dx + dy*dy;
          if (distSq < radius*radius) {
            float curr = coord(x,y,data1);
            float depress = radius/parm.maxCraterSize/height*(radius*radius-distSq)*255.0f/height/height;
            coord(x,y,data1) = (unsigned char)(max(0.0f, curr-depress));
          } else if (distSq < radius*radius*1.2f*1.2f) {
            float dist = sqrt(distSq);
            float theta = atan2(dy, dx);
            if (theta < 0) theta += 2*pi;
            float minThDist = pi;
            //Find the nearest ridge seed
            for (unsigned i=0; i<numRidges; ++i) {
              float diff = fabs(theta-ridges[i]);
              if (diff > pi) diff = pi*2-diff;
              if (diff < minThDist) minThDist = diff;
            }
            float delta = max(0.0f, 256*radius/height*max(0.0f,pi/12-minThDist)*12/pi
                                       *(1.0f-10.0f*fabs(dist/radius-1.1f)));
            float curr = coord(x,y,data1);
            if (curr == 0) delta /= 16;
            coord(x,y,data1) = (unsigned char)min(255.0f, curr+delta);
          }
        }
      }

      if (!isGeneratorRunning) {
        delete[] data1;
        return 0;
      }
    }

    /* Create rivers. For each river, pick a random point. If it is not land,
     * ignore and continue. Make that pixel water. Then, examine the 8 possible
     * directions randomly, by selecting 6 random numbers, and repeat the process
     * again. Do not run for more than width*8 iterations. Keep track of our previous
     * points, so we do not collide with self. Remember the last direction we went
     * when the new elevation was lower than the current. For this direction, we
     * use a looser test, so that rivers will actually go downhill over local planes.
     * This is an in-place transformation of data1.
     */
    {
      for (unsigned river=0; river<parm.rivers && isGeneratorRunning; ++river) {
        setProgress("Generating rivers", river/(float)parm.rivers);
        signed x=mtrand()%width;
        signed y=mtrand()%height;
        unsigned itersLeft = width*8;
        signed lastXO=mtrand()%3-1, lastYO=mtrand()%3-1;
        if (lastXO == 0 && lastYO == 0)
          lastYO = (y>height/2? -1 : +1);
        set<unsigned> prevPoints;
        while ((coord(x,y,data1) || prevPoints.count(x | (y << 16))) && itersLeft--) {
          unsigned currElev = coord(x,y,data1);
          prevPoints.insert(x | (y << 16));
          coord(x,y,data1)=0;

          static const signed dirs[8][2] = {
            {-1,-1}, {0,-1}, {+1,-1},
            {-1, 0},         {+1, 0},
            {-1,+1}, {0,+1}, {+1,+1},
          };
          signed mx=x+lastXO, my=y+lastYO;
          if (mx < 0) mx+=width;
          else if (mx >= width) mx-=width;
          if (my < 0) my=0;
          else if (my >= height) my=height-1;
          unsigned mine = currElev;
          for (unsigned i=0; i<6; ++i) {
            unsigned r=mtrand()&7;
            signed nx=x+dirs[r][0], ny=y+dirs[r][1];
            if (nx < 0) nx+=width;
            else if (nx >= width) nx -= width;
            if (ny < 0) continue;
            if (ny >= height) continue;
            if (prevPoints.count(nx | (ny << 16))) continue; //Already used

            unsigned elev = coord(nx,ny,data1);
            if (elev < mine) {
              mx = nx;
              my = ny;
              mine = elev;
              if (elev < currElev) {
                lastXO=dirs[r][0];
                lastYO=dirs[r][1];
              }
            }
          }
          x=mx;
          y=my;
        }
      }

      if (!isGeneratorRunning) {
        delete[] data1;
        return 0;
      }
    }

    /* Determine temperatures. The base temperature of a coordinate
     * is equatorTemperature-abs(y-solarEquator)*2*(equatorTemperature-polarTemperature).
     * If the pixel is water, add waterTemperatureDelta to it. Otherwise, add
     * elevation*altitudeTemperatureDelta to it.
     * This transforms data1 into data2.
     */
    elev_temp* data2=new elev_temp[width*height];
    {
      for (signed y=0; y<height && isGeneratorRunning; ++y) {
        setProgress("Calculating temperature", y/(float)height);
        float base = parm.equatorTemperature
                   - fabs(y/(float)height - parm.solarEquator)*2
                     *(parm.equatorTemperature-parm.polarTemperature);
        for (signed x=0; x<width; ++x) {
          unsigned char elev = coord(x,y,data1);
          coord(x,y,data2).elev = elev;
          if (elev)
            coord(x,y,data2).temp = (unsigned short)(base + elev/255.0f*parm.altitudeTemperatureDelta);
          else
            coord(x,y,data2).temp = (unsigned short)(base + parm.waterTemperatureDelta);
        }
      }

      delete[] data1;
      if (!isGeneratorRunning) {
        delete[] data2;
        return 0;
      }
    }

    /* Determine humidity. For each land pixel, pick 256 random points, with
     * probability distribution 0.01/distance^2, where distance is in heights.
     * For each point, if it is a water pixel, add vapourTransport/distance
     * to the humidity value. Then, pick 16 random points with even distribution
     * along the line between the current pixel and the water pixel. For each
     * of those points, if its altitude is higher than the current pixel's,
     * subtract mountainBlockage*(pointAltitude-pixelAltitude)/distance
     * from the humidity.
     * To improve performance, only calculate humidity for 1/16 points and
     * interpolate the rest.
     * Do not subtract more than what was added for all 16 points put together.
     * This transforms data2 into data3.
     */
    elev_temp_hum* data3;
    float maxHumidity=0;
    {
      data3 = new elev_temp_hum[width*height];
      for (signed y=0; y<height && isGeneratorRunning; y+=4) {
        setProgress("Calculating humidity", 0.9f*y/(float)height);
        for (signed x=0; x<width; x+=4) {
          coord(x,y,data3).elev = coord(x,y,data2).elev;
          coord(x,y,data3).temp = coord(x,y,data2).temp;
          if (!coord(x,y,data2).elev) {
            //Water
            coord(x,y,data3).hum=0;
          } else {
            //Land
            float localHumidity=parm.humidity;
            for (unsigned sample=0; sample<256; ++sample) {
              signed sx, sy;
              float distSq;
              do {
                sx = mtrand()%width;
                sy = mtrand()%height;
                float dx = x-sx, dy = y-sy;
                if (fabs(dx) > width/2) dx = width-fabs(dx);
                distSq = dx*dx + dy*dy;
              } while (mtrand()/(float)MTRAND_MAX > 1.0f - distSq/height/height);
              if (coord(sx,sy,data2).elev) continue;

              float sampleHumidity = 1.0f-distSq/height/height/parm.vapourTransport;

              float dx = sx-x, dy = sy-y;
              if (dx > width/2) dx = width-dx;
              else if (dx < -width/2) dx = -width-dx;
              for (unsigned subsample=0; subsample<16; ++subsample) {
                float off = mtrand()/(float)MTRAND_MAX;
                signed ssx = (signed)(x + dx*off);
                signed ssy = (signed)(y + dy*off);
                ssx %= width;
                //ssy will ALWAYS be in bounds
                if (coord(ssx,ssy,data2).elev > coord(x,y,data2).elev) {
                  float sdx = ssx-x, sdy=ssy-y;
                  if (fabs(sdx) > width/2) sdx = width-fabs(sdx);
                  float sdist = sqrt(sdx*sdx + sdy*sdy);
                  sampleHumidity -= parm.mountainBlockage
                                   *(coord(ssx,ssy,data2).elev-(float)coord(x,y,data2).elev)/255.0f
                                   /sdist*height*height;
                }
              }

              if (sampleHumidity > localHumidity) localHumidity = sampleHumidity;
            }
            coord(x,y,data3).hum = localHumidity;
            if (localHumidity > maxHumidity)
              maxHumidity = localHumidity;
          }
        }
      }

      if (!isGeneratorRunning) {
        delete[] data2;
        delete[] data3;
        return 0;
      }

      /* Interpolate remaining coordinates. */
      for (signed y=0; y<height && isGeneratorRunning; ++y) {
        setProgress("Calculating humidity", 0.9f + 0.1f*y/(float)height);
        signed y0 = y&~3;
        signed y1 = y0+4;
        if (y1 >= height) y1=y0;
        for (signed x=0; x<width; ++x) {
          signed x0 = (x&~3);
          if (x0 == x && y0 == y) continue;
          signed x1 = (x0+4)%width;
          coord(x,y,data3).temp = coord(x,y,data2).temp;
          if ((coord(x,y,data3).elev /* assign */ = coord(x,y,data2).elev)) {
            signed xo=x-x0, yo=y-y0;
            float h00 = coord(x0,y0,data3).hum,
                  h01 = coord(x0,y1,data3).hum,
                  h10 = coord(x1,y0,data3).hum,
                  h11 = coord(x1,y1,data3).hum;
            float h0 = h00*(1.0f-yo/4.0f) + h01*yo/4.0f;
            float h1 = h10*(1.0f-yo/4.0f) + h11*yo/4.0f;
            coord(x,y,data3).hum = h0*(1.0f-xo/4.0f) + h1*xo/4.0f;
          } else {
            coord(x,y,data3).hum = 0;
          }
        }
      }

      delete[] data2;

      if (!isGeneratorRunning) {
        delete[] data3;
        return 0;
      }
    }

    /* Temporary rasterisation pass.
     * Show temperatures, colourised to be appropriate
     * for Earth-like climates. If the temperature is
     * below freezing, set colour (0,0,(freezing-temp)*8).
     * Otherwise, set colour (temp-freezing, min(255,10*(temp-freezing)), 0).
     * For night, indicate raw altitude and water.
     */
    {
      resultDayImage = new png::image<png::rgb_pixel>(width, height);
      resultNightImage=new png::image<png::rgb_pixel>(width, height);
      png::image<png::rgb_pixel>& day(*resultDayImage), & night(*resultNightImage);
      unsigned char vr = (parm.vegitationColour >> 16) & 0xFF,
                    vg = (parm.vegitationColour >>  8) & 0xFF,
                    vb = (parm.vegitationColour >>  0) & 0xFF,
                    wr = (parm.waterColour      >> 16) & 0xFF,
                    wg = (parm.waterColour      >>  8) & 0xFF,
                    wb = (parm.waterColour      >>  0) & 0xFF,
                    ur = (parm.upperPlanetColour>> 16) & 0xFF,
                    ug = (parm.upperPlanetColour>>  8) & 0xFF,
                    ub = (parm.upperPlanetColour>>  0) & 0xFF,
                    lr = (parm.lowerPlanetColour>> 16) & 0xFF,
                    lg = (parm.lowerPlanetColour>>  8) & 0xFF,
                    lb = (parm.lowerPlanetColour>>  0) & 0xFF;
      for (signed row=0; row<height && isGeneratorRunning; ++row) {
        setProgress("Rasterising", row/(float)height);
        for (signed x=0; x<width; ++x) {
          unsigned short temp = coord(x,row,data3).temp;
          float hum = coord(x,row,data3).hum/maxHumidity;
          unsigned char elev = coord(x,row,data3).elev;
          if (elev) {
            if ((temp < parm.freezingPoint && mtrand()/(float)MTRAND_MAX > hum)
            ||  (temp >=parm.freezingPoint && hum < parm.vegitationHumidity)) {
              //Exposed surface
              day[row][x] = png::rgb_pixel((unsigned char)(ur*(elev/255.0f) + lr*(1.0f-elev/255.0f)),
                                           (unsigned char)(ug*(elev/255.0f) + lg*(1.0f-elev/255.0f)),
                                           (unsigned char)(ub*(elev/255.0f) + lb*(1.0f-elev/255.0f)));
            } else if (temp < parm.freezingPoint) {
              //Ice
              day[row][x] = png::rgb_pixel(0xFF,0xFF,0xFF);
            } else {
              //Vegitation
              day[row][x] = png::rgb_pixel(vr,vg,vb);
            }
          } else {
            if (temp < parm.freezingPoint) {
              //Ice
              day[row][x] = png::rgb_pixel(0xFF,0xFF,0xFF);
            } else {
              //Water
              day[row][x] = png::rgb_pixel(wr,wg,wb);
            }
          }

          if (elev == 0)
            night[row][x] = png::rgb_pixel(0,0,0xFF);
          else if (elev < 0x80)
            night[row][x] = png::rgb_pixel(elev*2,0xFF,0);
          else
            night[row][x] = png::rgb_pixel(0xFF,0xFF,(elev-0x80)*2);
        }
      }
      delete[] data3;
    }
    isGeneratorRunning=false;
    return 0;
  }
}
