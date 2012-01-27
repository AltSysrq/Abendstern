/**
 * @file
 * @author Jason Lingle
 * @brief Prototypes for ASGI functions.
 * @see asgi.txt
 */

/*
 * asgi.hxx
 *
 *  Created on: 22.01.2011
 *      Author: jason
 */

#ifndef ASGI_HXX_
#define ASGI_HXX_

#include <exception>
#include <stdexcept>

#include "vec.hxx"

/**
 * Contains functions and definitions for the Abendstern Scripting Graphics Interface.
 * @see asgi.txt
 */
namespace asgi {
  /** Thrown when an ASGI function is used improperly. */
  class AsgiException: public std::runtime_error {
    public:
    /** Constructs a new AsgiException with the given description. */
    AsgiException(const char*);
  };

  /**
   * Defines the types of primitives that ASGI can draw.
   */
  enum Primitive {
    /** Equivilant of GL_POINTS */
    Points,
    /** Equivilant of GL_LINES */
    Lines,
    /** Equivilant of GL_LINE_STRIP */
    LineStrip,
    /** Equivilant of GL_LINE_LOOP */
    LineLoop,
    /** Equivilant of GL_TRIANGLES */
    Triangles,
    /** Equivilant of GL_TRIANGLE_STRIP */
    TriangleStrip,
    /** Equivilant of GL_TRIANGLE_FAN */
    TriangleFan,
    /** Equivilant of GL_QUADS */
    Quads
  };

  /** Equivilant of glBegin(GL_ENUM) */
  void begin(Primitive);
  /** Equivilant of glEnd */
  void end();
  /** Equivilant of glVertex2f(float,float) */
  void vertex(float,float);
  /** Equivilant of glColor4f(float,float,float,float) */
  void colour(float,float,float,float);
  /** Equivilant of glPushMatrix() */
  void pushMatrix();
  /** Equivilant of glPopMatrix() */
  void popMatrix();
  /** Equivilant of glLoadIdentity() */
  void loadIdentity();
  /** Equivilant of glTranslatef(x,y,0.0f) */
  void translate(float,float);
  /** Equivilant of glRotatef(rot,0.0f,0.0f,1.0f) */
  void rotate(float);
  /** Equivilant of glScalef(x,y,1.0f) */
  void scale(float,float);
  /** Equivilant of glScalef(s,s,1.0f) */
  void uscale(float);
  /**
   * Performs a full reset of ASGI.
   * This needs to be called at least once before ASGI is used.
   * Calling it guarantees that ASGI will be in a known, valid
   * state, and will not be drawing primitives (ie, logically
   * outside a begin/end sequence).
   */
  void reset();

  /** Returns the current ASGI colour. */
  const vec4& getColour();
}

#endif /* ASGI_HXX_ */
