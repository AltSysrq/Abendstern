/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/gl32emu.hxx
 */

/*
 * gl32emu.cxx
 *
 *  Created on: 15.02.2011
 *      Author: jason
 */

//Only compile if in compatibility mode
//But do include stuff for Doxygen
#if defined(AB_OPENGL_14) || defined(DOXYGEN)

#include <SDL.h>

#include <GL/gl.h>

#include <cstdlib>
#include <cstring>
#include <map>
#include <iostream>

#include "gl32emu.hxx"
#include "src/globals.hxx"

//End includes for Doxygen
#endif
#ifdef AB_OPENGL_14

using namespace std;

namespace gl32emu {
  struct VertexBufferObject {
    /* We can't set the lists up until we have all data
     * (which could come in two pieces) and the format for that
     * data.
     * Copy data here until it can be processed.
     * If this is non-NULL, the draw list must still be created.
     */
    unsigned char* arrayBufferData;
    unsigned char* indexBufferData;
    unsigned arrayBufferLength;
    unsigned indexBufferLength;

    unsigned vertexSize;
    /* Each offset is defined only if its nfloats is non-zero. */
    unsigned vertexOffset, vertexNFloats;
    unsigned colourOffset, colourNFloats;
    unsigned texcooOffset, texcooNFloats;
  };

  static map<GLuint,VertexBufferObject> vbos;

  static GLuint currentVBO;

  void init() {
    currentVBO=0;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }

  /* Create the current display list if there are
   * pending changes. Then, delete and nullify
   * all information. Do not change currentVBO.
   */
  static void finilizeData(VertexBufferObject& v, GLenum drawMode) {
    if (!v.arrayBufferData) return;

    unsigned baseMask = ~0;
    unsigned indexMask = 0;
    unsigned indexStride = 0;
    unsigned int dummy[1];
    unsigned char* indices=(unsigned char*)dummy;
    unsigned numVerts = v.arrayBufferLength / v.vertexSize;

    //If there is an index buffer, figure out its format
    if (v.indexBufferData) {
      indices=v.indexBufferData;

      if (v.indexBufferLength % sizeof(GLuint) == 0) {
        //Might be unsigned int
        GLuint* indices32=(GLuint*)v.indexBufferData;
        for (unsigned i=0; i<v.indexBufferLength / sizeof(GLuint) && indices32; ++i)
          if (indices32[i] >= v.arrayBufferLength / v.vertexSize) indices32=NULL; //Nope

        if (indices32) {
          //32-bit unsigned format
          indices = v.indexBufferData;
          numVerts = v.indexBufferLength / sizeof(GLuint);
          indexMask = ~0;
          baseMask = 0;
          indexStride = 4;
        }
      }

      if (baseMask && v.indexBufferLength % sizeof(unsigned short) == 0) {
        //Might be unsigned short
        unsigned short* indices16=(unsigned short*)v.indexBufferData;
        for (unsigned i=0; i<v.indexBufferLength / sizeof(unsigned short) && indices16; ++i)
          if (indices16[i] >= v.arrayBufferLength / v.vertexSize) indices16=NULL; //Out of range

        if (indices16) {
          //16-bit unsigned format
          //If this is big-endian, we need to move back 2 bytes
          #if SDL_BYTEORDER == SDL_BIG_ENDIAN
          indices = v.indexBufferData-2;
          #else
          indices = v.indexBufferData;
          #endif
          baseMask = 0;
          indexMask = 0xFFFF;
          indexStride = 2;
          numVerts = v.indexBufferLength / sizeof(unsigned short);
        }
      }

      if (baseMask) {
        //The only option left is unsigned char
        //On big-endian, we must move back 3 bytes
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        indices = v.indexBufferData-3;
        #else
        indices = v.indexBufferData;
        #endif
        baseMask = 0;
        indexMask = 0xFF;
        indexStride = 2;
        numVerts = v.indexBufferLength;
      }
    }

    glNewList(currentVBO, GL_COMPILE);
    glBegin(drawMode);
    for (unsigned i=0; i<numVerts; ++i, indices += indexStride) {
      unsigned char* vertex = v.arrayBufferData + v.vertexSize*((i & baseMask) + (*(GLuint*)indices & indexMask));
      if (v.colourNFloats == 3)
        glColor3fv((GLfloat*)(vertex + v.colourOffset));
      else if (v.colourNFloats == 4)
        glColor4fv((GLfloat*)(vertex + v.colourOffset));
      if (v.texcooNFloats == 2)
        glTexCoord2fv((GLfloat*)(vertex + v.texcooOffset));
      else if (v.texcooNFloats == 3)
        glTexCoord3fv((GLfloat*)(vertex + v.texcooOffset));
      else if (v.texcooNFloats == 4)
        glTexCoord4fv((GLfloat*)(vertex + v.texcooOffset));
      if (v.vertexNFloats == 2)
        glVertex2fv((GLfloat*)(vertex + v.vertexOffset));
      else if (v.vertexNFloats == 3)
        glVertex3fv((GLfloat*)(vertex + v.vertexOffset));
      else if (v.vertexNFloats == 4)
        glVertex4fv((GLfloat*)(vertex + v.vertexOffset));
    }
    glEnd();
    glEndList();

    //Clean up
    delete[] v.arrayBufferData;
    if (v.indexBufferData)
      delete[] v.indexBufferData;
    v.arrayBufferData=v.indexBufferData=NULL;
  }

  void genVertexArrays(unsigned n, GLuint* dst) {
    static unsigned nextVAO=1;
    while (n--) *dst++ = nextVAO++;
  }

  void deleteVertexArrays(unsigned, const GLuint*) {}

  void genBuffers(unsigned n, GLuint* dst) {
    unsigned base=glGenLists(n);
    VertexBufferObject init;
    init.arrayBufferData = init.indexBufferData=NULL;
    init.vertexSize=0;
    init.vertexNFloats=init.colourNFloats=init.texcooNFloats=0;
    init.vertexOffset=init.colourOffset=init.texcooOffset=0;
    for (unsigned i=0; i<n; ++i) vbos[base+i] = init;
    while (n--) *dst++ = base++;
  }

  void deleteBuffers(unsigned n, const GLuint* dst) {
    glDeleteLists(*dst, n);
    for (unsigned i=0; i<n; ++i) {
      if (vbos[i].arrayBufferData) delete[] vbos[i].arrayBufferData;
      if (vbos[i].indexBufferData) delete[] vbos[i].indexBufferData;
    }
    while (n--) vbos.erase(vbos.find(*dst++));
  }

  void bindVertexArray(GLuint) {}

  void bindBuffer(GLenum t, GLuint b) {
    if (t == GL_ARRAY_BUFFER)
      currentVBO=b;
  }

  void bufferData(GLenum t, GLsizei nbytes, const GLvoid* data, GLenum) {
    VertexBufferObject& vbo = vbos[currentVBO];
    unsigned char*& tgtptr=(t == GL_ARRAY_BUFFER? vbo.arrayBufferData : vbo.indexBufferData);
    unsigned& tgtlen      =(t == GL_ARRAY_BUFFER? vbo.arrayBufferLength : vbo.indexBufferLength);

    if (tgtptr) delete[] tgtptr;
    tgtptr = new unsigned char[nbytes];
    memcpy(tgtptr, data, nbytes);
    tgtlen = nbytes;
  }

  void drawArrays(GLenum mode, GLuint, GLuint) {
    finilizeData(vbos[currentVBO], mode);
    glCallList(currentVBO);
  }

  void setBufferVertexSize(unsigned n) {
    vbos[currentVBO].vertexSize=n;
  }

  void setBufferAttributeInfo(unsigned off, unsigned nf, VertexAttribute attr) {
    VertexBufferObject& vbo = vbos[currentVBO];
    switch (attr) {
      case Position:
        vbo.vertexOffset=off;
        vbo.vertexNFloats=nf;
        break;
      case Colour:
        vbo.colourOffset=off;
        vbo.colourNFloats=nf;
        break;
      case TextureCoordinate:
        vbo.texcooOffset=off;
        vbo.texcooNFloats=nf;
        break;
    }
  }

  void setShaderEmulation(ShaderEmulation emul) {
    static const GLubyte stippleMask[4*32] = {
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
      0xAA,0xAA,0xAA,0xAA,
      0x55,0x55,0x55,0x55,
    };
    switch (emul) {
      case SE_quick:
      case SE_simpleColour:
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LINE_STIPPLE);
        glDisable(GL_POLYGON_STIPPLE);
        break;
      case SE_textureReplace:
        glDisable(GL_LINE_STIPPLE);
        glDisable(GL_POLYGON_STIPPLE);
        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        break;
      case SE_textureModulate:
      case SE_textureStensil:
        glDisable(GL_LINE_STIPPLE);
        glDisable(GL_POLYGON_STIPPLE);
        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        break;
      case SE_stipleColour:
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LINE_STIPPLE);
        glEnable(GL_POLYGON_STIPPLE);
        glLineStipple(1, 0xAAAA);
        glPolygonStipple(stippleMask);
        break;
      case SE_stipleTexture:
        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_LINE_STIPPLE);
        glEnable(GL_POLYGON_STIPPLE);
        glLineStipple(1, 0xAAAA);
        glPolygonStipple(stippleMask);
        break;
    }
  }
}

#endif /* AB_OPENGL_14 */
