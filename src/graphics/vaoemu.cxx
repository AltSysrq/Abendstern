/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/vaoemu.hxx
 */

/*
 * vaoemu.cxx
 *
 *  Created on: 14.11.2011
 *      Author: jason
 */

#ifdef AB_OPENGL_21

#define VAOEMU_CXX_

#include <vector>
#include <cstring>

#include <GL/gl.h>

#include "vaoemu.hxx"

using namespace std;

namespace vaoemu {
  class VertexArrayOperation {
    public:
    virtual void exec() = 0;
    virtual ~VertexArrayOperation() {}
  };
  class VAO_VertexAttribPointer: public VertexArrayOperation {
    GLuint a, b;
    GLenum c;
    GLboolean d;
    GLsizei e;
    const GLvoid* f;

    public:
    VAO_VertexAttribPointer(GLuint aa, GLuint bb, GLenum cc, GLenum dd, GLsizei ee,
                            const GLvoid* ff)
    : a(aa), b(bb), c(cc), d(dd), e(ee), f(ff)
    {
      exec();
    }
    virtual ~VAO_VertexAttribPointer() {
    }

    virtual void exec() {
      glVertexAttribPointer(a,b,c,d,e,f);
    }
  };
  class VAO_EnableVertexAttribArray: public VertexArrayOperation {
    GLuint a;
    public:
    VAO_EnableVertexAttribArray(GLuint aa)
    : a(aa)
    {
      exec();
    }

    virtual void exec() {
      glEnableVertexAttribArray(a);
    }
  };

  class VertexArray {
    public:
    vector<VertexArrayOperation*> ops;
    //Free arrays are stored as a linked list
    unsigned nextFree;

    VertexArray()
    : nextFree(0)
    { }

    ~VertexArray() {
      destroy();
    }

    void destroy() {
      for (unsigned i=0; i<ops.size(); ++i)
        delete ops[i];
      ops.clear();
    }

    void free(unsigned freeList) {
      destroy();
      nextFree = freeList;
    }

    void activate() const {
      //Disable all attributes
      for (unsigned i=0; i<32; ++i)
        glDisableVertexAttribArray(i);
      //Perform actions
      for (unsigned i=0; i<ops.size(); ++i)
        ops[i]->exec();
    }
  };

  static vector<VertexArray*> vertexArrays;
  static unsigned freeList = -1; //Index, out of bounds = none
  static VertexArray*currentArray;

  void vertexAttribPointer(GLuint a, GLuint b, GLenum c, GLboolean d, GLsizei e,
                           const GLvoid* f) {
    currentArray->ops.push_back(new VAO_VertexAttribPointer(a,b,c,d,e,f));
  }
  void enableVertexAttribArray(GLuint i) {
    currentArray->ops.push_back(new VAO_EnableVertexAttribArray(i));
  }

  void bindVertexArray(GLuint i) {
    currentArray = vertexArrays[i-1];
    currentArray->activate();
  }

  void genVertexArrays(GLsizei cnt, GLuint* dst) {
    while (cnt--) {
      if (freeList < vertexArrays.size()) {
        *dst++ = freeList+1;
        freeList = vertexArrays[freeList]->nextFree;
      } else {
        *dst++ = vertexArrays.size()+1;
        vertexArrays.push_back(new VertexArray);
      }
    }
  }

  void deleteVertexArrays(GLsizei cnt, const GLuint* dst) {
    while (cnt--) {
      vertexArrays[*dst-1]->free(freeList);
      freeList = *dst-1;
      ++dst;
    }
  }

  void resetVAO() {
    currentArray->destroy();
  }

  void bindBuffer(GLenum t, GLuint buff) {
    glBindBuffer(t,buff);
    if (t == GL_ARRAY_BUFFER) currentArray->activate();
  }
}

#endif /* AB_OPENGL_21 */
