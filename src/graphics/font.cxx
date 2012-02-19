/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/font.hxx
 */

#include <cstdlib>
#include <iostream>
#include <cstring>
#include <stack>

#include <GL/gl.h>
#include <SDL.h>
#include <SDL_image.h>
//Use Tcl's UTF-8 decoder
#include <tcl.h>

#include "font.hxx"
#include "vec.hxx"
#include "shader.hxx"
#include "cmn_shaders.hxx"
#include "matops.hxx"
#include "glhelp.hxx"
#include "asgi.hxx"
#include "gl32emu.hxx"
#include "src/globals.hxx"
#include "src/exit_conditions.hxx"
using namespace std;

static shader::textureStensilU uniform;

/* Emulate the old GL's colour stack */
static stack<vec4> colourStack;

inline float toScreen(int v) {
  return v/(float)screenW;
}

Font::Font(const char* _filename, float virtHeight, bool stip)
: stipple(stip), height(renderFont(_filename, virtHeight)),
  filename(_filename), hasVAO(false)
{
}

Font::~Font() {
  if (headless) return;
  for (unsigned i=0; i<lenof(tex); ++i)
    if (tex[i])
      glDeleteTextures(1, &tex[i]);
}

float Font::width(char ch) const noth {
  return toScreen(widths[ch+128]);
}

float Font::width(const char* str) const noth {
  int total=0;
  bool forceWidthNext = false;
  unsigned forceWidth=0; //Initialise to suppress compiler warning
  while (*str) {
    if (*str == '\a') {
      //Special sequence
      switch (*(str+1)) {
        case '[': {
          //Skip either 10 characters, or until after the next close-paren
          if (*(str+2) == '(') {
            str+=3;
            while (*str != ')' && *str) ++str;
            if (!*str) goto endOfString; //Hit unexpected NUL
            ++str; //Skip close paren
          } else {
            unsigned toSkip=10;
            //Check for term NUL, so we can't be crashed via \a[F
            while (*++str && --toSkip) /*doNothing()*/;
          }
        } break;
        case ']': {
          str+=2; //Skip "\a]"
        } break;
        case '_': {
          str+=2; //Skip \a_
          if (*str) {
            forceWidthNext = true;
            unsigned ch = *str;
            if (!hasLoaded[ch]) const_cast<Font*>(this)->renderCharacter(ch);
            forceWidth = widths[ch];
            ++str;
          }
        } break;
        case '\a': {
          //Pretend end-of-string
          goto endOfString;
        } break;
        case '{':
        case '}':
        case '&': str += 2; break; //Skip
        default: ++str; //Ignore isolated "\a"
      }
    } else {
      Tcl_UniChar ch;
      str += Tcl_UtfToUniChar(str, &ch);
      ch &= 0xFFFF;
      if (!hasLoaded[ch]) const_cast<Font*>(this)->renderCharacter(ch);
      total += (!forceWidthNext? widths[ch] : forceWidth);
      forceWidthNext = false;
    }
  }
  endOfString:
  return toScreen(total)*mult;
}

float Font::getHeight() const noth {
  return toScreen(height)*mult;
}

float Font::getRise() const noth {
  return toScreen(rise)*mult;
}

float Font::getDip() const noth {
  return toScreen(dip)*mult;
}

void Font::preDraw() const noth {
  vec4 colour=asgi::getColour();
  preDraw(colour[0], colour[1], colour[2], colour[3]);
}

void Font::preDraw(float r, float g, float b, float a) const noth {
  if (!hasVAO) {
    vao = newVAO();
    glBindVertexArray(vao);
    vbo = newVBO();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    shader::textureStensilV vertices[4] = {
        { {{0, 0        , 0, 1}}, {{0,1}} },
        { {{1, 0        , 0, 1}}, {{1,1}} },
        { {{0, 1        , 0, 1}}, {{0,0}} },
        { {{1, 1        , 0, 1}}, {{1,0}} },
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    shader::textureStensil->setupVBO();
    hasVAO=true;
  }

  uniform.modColour = Vec4(r,g,b,a);
  uniform.colourMap = 0;
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  shader::textureStensil->activate(&uniform);
}

void Font::postDraw() const noth {
}

void Font::draw(char ch, float x, float y) const noth {
  const char str[2] = { ch, 0 };
  draw(str, x, y);
};

void Font::draw(const char* str, float x, float y, float maxw, bool dontClear, const float* forceW) const noth {
  BEGINGP("Font")
  y-=getHeight() - toScreen(powerHeight)*mult;
  float sheight = toScreen(powerHeight)*mult;
  unsigned pushLevel=0;
  bool forceWidthNext = false;
  float forceWidth = 0; //Initialise to suppress compiler warning
  bool blink = false;
  while (*str && x<maxw) {
    if (*str == '\a') {
      ++str;
      switch (*str) {
        case '[': {
          ++str;
          if (*str == '(') {
            ++str;
            string name;
            //Restrict length of name, and check for premature NUL as well
            while (*str != ')' && *str && name.size()<20) {
              name.push_back(*str);
              ++str;
            }
            if (!*str) goto exit_loop;

            ++str; //Skip close paren

            //Get the colour
            float r,g,b,a;
            try {
              libconfig::Setting& s(conf["conf"]["hud"]["colours"][name.c_str()]);
              r=s[0];
              g=s[1];
              b=s[2];
              a=s[3];
              colourStack.push(uniform.modColour);
              uniform.modColour=Vec4(r,g,b,a);
              ++pushLevel;
            } catch (...) {
              //Ignore
            }
          } else {
            //Read hex
            Uint32 rgba=0;
            bool ok=true; //Easy way to skip the rest if there's an error
            for (int i=0; i<8 && *str; ++i, ++str) if (ok) {
              char ch=*str;
              Uint32 val=0;
              if ('0'<=ch && ch<='9') val=ch-'0';
              else if ('A'<=ch && ch<='F') val=ch-'A'+10;
              else if ('a'<=ch && ch<='f') val=ch-'a'+10;
              else ok=false;
              rgba = (rgba << 4) | val;
            }

            if (!*str) goto exit_loop; //Premature term NUL

            if (ok) {
              float r = ((rgba >> 24) & 0xFF)/256.0f,
                    g = ((rgba >> 16) & 0xFF)/256.0f,
                    b = ((rgba >> 8 ) & 0xFF)/256.0f,
                    a = ((rgba      ) & 0xFF)/256.0f;
              colourStack.push(uniform.modColour);
              uniform.modColour=Vec4(r,g,b,a);
              ++pushLevel;
            }
          }
        } break;
        case ']': {
          ++str; //Skip bracket
          if (pushLevel) {
            uniform.modColour=colourStack.top();
            colourStack.pop();
            --pushLevel;
          }
        } break;
        case '_': {
          ++str; //Skip underscore
          if (*str && !forceW) {
            forceWidthNext = true;
            unsigned ch = *str;
            if (!hasLoaded[ch]) const_cast<Font*>(this)->renderCharacter(ch);
            forceWidth = toScreen(widths[ch])*mult;
          }
          if (*str) ++str;
        } break;
        case '\a': {
          //Pretend end of string
          goto exit_loop;
        } break;
        case '&': ++str; break; //Skip
        case '{': blink = true; ++str; break;
        case '}': blink = false; ++str; break;
        //Ignore default
      }

      continue;
    }

    Tcl_UniChar ch;
    str += Tcl_UtfToUniChar(str, &ch);
    ch &= 0xFFFF;
    if (!hasLoaded[ch]) const_cast<Font*>(this)->renderCharacter(ch);
    float swidth=toScreen(widths[ch])*mult;
    if (!blink || ((SDL_GetTicks() & 1023) < 512)) {
      glBindTexture(GL_TEXTURE_2D, tex[ch]);
      mPush();
      mTrans(x,y);
      mScale(toScreen(powerWidths[ch])*mult, sheight);
      shader::textureStensil->setUniforms(&uniform);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      mPop();
    }
    x += (forceW? *forceW : forceWidthNext? forceWidth : swidth);
    forceWidthNext = false;
  }

  exit_loop:
  if (!dontClear) {
    while (pushLevel--) {
      uniform.modColour=colourStack.top();
      colourStack.pop();
    }
  }
  ENDGP
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  Uint32 rmask=0xff000000,
         gmask=0x00ff0000,
         bmask=0x0000ff00,
         amask=0x000000ff;
#else
  Uint32 rmask=0x000000ff,
         gmask=0x0000ff00,
         bmask=0x00ff0000,
         amask=0xff000000;
#endif

int Font::renderFont(const char* dirname, float virtHeight) const noth {
  char filename[256];

  if (headless) return (int)(virtHeight*screenH); //Arbitrary number that doesn't matter

  //Here's where we cheat :)
  unsigned* widths = const_cast<unsigned*>(this->widths);
  unsigned* powerWidths = const_cast<unsigned*>(this->powerWidths);
  GLuint* tex = const_cast<GLuint*>(this->tex);
  memset(const_cast<bool*>(this->hasLoaded), 0, sizeof(this->hasLoaded));

  //Gather information from the M character
  sprintf(filename, "%s/004d.png", dirname);
  SDL_Surface* msurf = IMG_Load(filename);
  unsigned realHeight = msurf->h;
  const_cast<Font*>(this)->height = realHeight;
  const_cast<Font*>(this)->mult = virtHeight / toScreen(realHeight);
  unsigned lowestOpaque=0;
  for (unsigned i=0; i<(unsigned)(msurf->h * msurf->w); i += msurf->format->BytesPerPixel)
    if ((*(Uint32*)(((unsigned char*)msurf->pixels)+i)) & msurf->format->Amask)
      lowestOpaque = i / msurf->w / msurf->format->BytesPerPixel;

  const_cast<Font*>(this)->rise = realHeight - lowestOpaque;
  const_cast<Font*>(this)->dip = lowestOpaque;

  SDL_FreeSurface(msurf);

  //Bring height to next power of two for OpenGL
  unsigned powerHeight=1;
  while (powerHeight < realHeight) powerHeight *= 2;
  const_cast<Font*>(this)->powerHeight = powerHeight;

  //Never any control chars
  for (unsigned i=0; i<32; ++i) {
    tex[i]=0;
    widths[i] = 0;
    powerWidths[i] = 0;
  }

  return realHeight;
}

void Font::renderCharacter(unsigned ch) noth {
  hasLoaded[ch] = true;

  const char* dirname = this->filename;
  char filename[256];

  //Only doing one character now
  //for (; ch<0x10000 && SDL_GetTicks() < start+50; ++ch) {
    sprintf(filename, "%s/%04x.png", dirname, ch);
    SDL_Surface* surf = IMG_Load(filename);
    if (!surf) {
      widths[ch]=0;
      tex[ch]=0;
      powerWidths[ch]=0;
      return; //continue; //No image for this character
    }

    widths[ch]=surf->w;

    //Power-of-two width
    powerWidths[ch]=1;
    while (powerWidths[ch] < (unsigned)surf->w) powerWidths[ch] *= 2;

    unsigned width = powerWidths[ch];
    if (!headless) {
      //GL-compatible surface
      SDL_Surface* glCompat=SDL_CreateRGBSurface(SDL_SWSURFACE, width, powerHeight, 32, rmask, gmask, bmask, amask);
      SDL_SetAlpha(surf, 0, 0);
      SDL_BlitSurface(surf, 0, glCompat, 0);
      //Handle stippling
      if (stipple) {
        for (unsigned y=0; y<powerHeight; ++y)
          for (unsigned x=0; x<width; ++x)
            if ((((unsigned)(x*mult))+((unsigned)(y*mult)))&1)
              for (unsigned i=0; i<4; ++i)
                ((GLubyte*)glCompat->pixels)[y*width*4+x*4+i]=0;
      }
      //Over to OpenGL
      glGenTextures(1, &tex[ch]);
      glBindTexture(GL_TEXTURE_2D, tex[ch]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, glCompat->w, glCompat->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, glCompat->pixels);

      //Free memory
      SDL_FreeSurface(glCompat);
    }
    SDL_FreeSurface(surf);
  //} commented loop
}

static char sysfontData[4][sizeof(Font)];
Font*const sysfont(reinterpret_cast<Font*>(sysfontData[0])),
    *const sysfontStipple(reinterpret_cast<Font*>(sysfontData[1])),
    *const smallFont(reinterpret_cast<Font*>(sysfontData[2])),
    *const smallFontStipple(reinterpret_cast<Font*>(sysfontData[3]));
