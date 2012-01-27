/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/graphics/acsgi.hxx
 */

/*
 * acsgi.cxx
 *
 *  Created on: 10.10.2011
 *      Author: jason
 */

#include <vector>
#include <string>

#include "asgi.hxx"
#include "font.hxx"
#include "acsgi.hxx"
#include "vec.hxx"

using namespace std;

namespace acsgi {

  //Classes representing each operation
  class Op {
    public:
    virtual void apply() = 0;
    virtual ~Op() {}
  };

  class Begin: public Op {
    asgi::Primitive what;

    public:
    Begin(asgi::Primitive p)
    : what(p)
    { }
    virtual void apply() {
      asgi::begin(what);
    }
  };

  class End: public Op {
    public:
    virtual void apply() {
      asgi::end();
    }
  };

  class Vertex: public Op {
    float x, y;

    public:
    Vertex(float a0, float a1)
    : x(a0), y(a1)
    { }
    virtual void apply() {
      asgi::vertex(x,y);
    }
  };

  class Colour: public Op {
    float r, g, b, a;

    public:
    Colour(float fr, float fg, float fb, float fa)
    : r(fr), g(fg), b(fb), a(fa)
    { }

    virtual void apply() {
      asgi::colour(r,g,b,a);
    }
  };

  class PushMatrix: public Op {
    public:
    virtual void apply() {
      asgi::pushMatrix();
    }
  };

  class PopMatrix: public Op {
    public:
    virtual void apply() {
      asgi::popMatrix();
    }
  };

  class LoadIdentity: public Op {
    public:
    virtual void apply() {
      asgi::loadIdentity();
    }
  };

  class Translate: public Op {
    float x, y;

    public:
    Translate(float a, float b)
    : x(a), y(b)
    { }

    virtual void apply() {
      asgi::translate(x,y);
    }
  };

  class Rotate: public Op {
    float theta;

    public:
    Rotate(float t)
    : theta(t)
    { }

    virtual void apply() {
      asgi::rotate(theta);
    }
  };

  class Scale: public Op {
    float x, y;

    public:
    Scale(float a, float b)
    : x(a), y(b)
    { }

    virtual void apply() {
      asgi::scale(x,y);
    }
  };

  class UScale: public Op {
    float x;

    public:
    UScale(float a)
    : x(a)
    { }

    virtual void apply() {
      asgi::uscale(x);
    }
  };

  static bool normalText = true;

  class Text: public Op {
    string txt;
    float x, y;

    public:
    Text(const char* str, float a, float b)
    : txt(str), x(a), y(b)
    { }

    virtual void apply() {
      if (normalText) {
        sysfont->preDraw();
        sysfont->draw(txt.c_str(), x, y);
        sysfont->postDraw();
      } else {
        vec4 colour(asgi::getColour());
        sysfontStipple->preDraw(colour[0], colour[1], colour[2], 1);
        sysfontStipple->draw(txt.c_str(), x, y);
        sysfontStipple->postDraw();
      }
    }
  };

  static vector<Op*> ops;
}

void acsgi_begin() {
  for (unsigned i=0; i<acsgi::ops.size(); ++i)
    delete acsgi::ops[i];
  acsgi::ops.clear();
}

void acsgi_end() {
}

void acsgi_draw() {
  for (unsigned i=0; i<acsgi::ops.size(); ++i)
    acsgi::ops[i]->apply();
}

void acsgi_textNormal(bool b) {
  acsgi::normalText = b;
}

static inline void a(acsgi::Op* op) {
  acsgi::ops.push_back(op);
}

void acsgid_begin(asgi::Primitive what) {
  a(new acsgi::Begin(what));
}

void acsgid_end() {
  a(new acsgi::End);
}

void acsgid_vertex(float x, float y) {
  a(new acsgi::Vertex(x,y));
}

void acsgid_colour(float r, float g, float b, float alpha) {
  a(new acsgi::Colour(r,g,b,alpha));
}

void acsgid_pushMatrix() {
  a(new acsgi::PushMatrix);
}

void acsgid_popMatrix() {
  a(new acsgi::PopMatrix);
}

void acsgid_loadIdentity() {
  a(new acsgi::LoadIdentity);
}

void acsgid_translate(float x, float y) {
  a(new acsgi::Translate(x,y));
}

void acsgid_rotate(float t) {
  a(new acsgi::Rotate(t));
}

void acsgid_scale(float x, float y) {
  a(new acsgi::Scale(x,y));
}

void acsgid_uscale(float x) {
  a(new acsgi::UScale(x));
}

void acsgid_text(const char* str, float x, float y) {
  a(new acsgi::Text(str, x, y));
}
