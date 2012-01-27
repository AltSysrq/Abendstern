/**
 * @file
 * @author Jason Lingle
 * @brief Defines the Abendstern Compiled Scripting Graphics Interface.
 *
 * There are a number of cases within the Abendstern GUI where the normal,
 * dynamic drawing method results in an unacceptable frame-rate drop, though
 * nothing actually must be changed (eg, the stats screen within the game
 * results in an 80% frame-rate drop).
 *
 * The functions within this file allow defining a single compiled, static
 * drawing run that can be used in these circumstances.
 *
 * This is ultimately a front-end to ASGI, and therefore also takes a meaningful
 * speed hit on non-OpenGL-1.1 versions.
 *
 * It may also request that the font use stippling (and no transparency).
 */

/*
 * acsgi.hxx
 *
 *  Created on: 10.10.2011
 *      Author: jason
 */

#ifndef ACSGI_HXX_
#define ACSGI_HXX_

#include "asgi.hxx"

/**
 * Discards the current ACSGI object and begins a new one.
 *
 * Calls to ACSGI drawing functions between this call and the corresponding
 * acsgi_end will be added to the object.
 */
void acsgi_begin();

/**
 * Finalises the current ACSGI object and makes it ready for drawing.
 * After this call, the effects of ACSGI drawing functions are undefined.
 */
void acsgi_end();

/**
 * Draws the current ACSGI object.
 * This cannot be used within begin...end invocations.
 */
void acsgi_draw();

/**
 * Sets whether normal font drawing is used.
 * If true, fonts are smooth and semitransparent.
 * If false, they are stippled and opaque.
 */
void acsgi_textNormal(bool);

/**
 * Corresponds to asgi::begin().
 */
void acsgid_begin(asgi::Primitive);

/** Corresponds to asgi::end(). */
void acsgid_end();

/** Corresponds to asgi::vertex(). */
void acsgid_vertex(float,float);

/** Corresponds to asgi::colour(). */
void acsgid_colour(float,float,float,float);

/** Corresponds to asgi::pushMatrix(). */
void acsgid_pushMatrix();

/** Corresponds to asgi::popMatrix(). */
void acsgid_popMatrix();

/** Corresponds to asgi::loadIdentity(). */
void acsgid_loadIdentity();

/** Corresponds to asgi::translate(float,float). */
void acsgid_translate(float, float);

/** Corresponds to asgi::rotate(float). */
void acsgid_rotate(float);

/** Corresponds to asgi::scale(float,float). */
void acsgid_scale(float,float);

/** Corresponds to asgi::uscale(float,float). */
void acsgid_uscale(float);

/** Draws the given text in the standard font at the requested location. */
void acsgid_text(const char*, float, float);

#endif /* ACSGI_HXX_ */
