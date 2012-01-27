/**
 * @file
 * @author Jason Lingle
 * @brief Contains the Font class
 */

#ifndef FONT_HXX_
#define FONT_HXX_
#include <GL/gl.h>
#include <SDL.h>

#include "src/opto_flags.hxx"
#include "src/core/aobject.hxx"

/** The Font class encapsulates a pre-rendered
 * true-type font.
 * It is loaded from all images of
 * the format<br>
 * &nbsp;  <code>%s/%04x.png        filename, uint16-char</code><br>
 * that exist. The rise and dip are determined by examining
 * the bitmap of the 'M' character.
 */
class Font: public AObject {
  private:
  GLuint tex            [0x10000];
  bool stipple;
  unsigned widths       [0x10000];
  unsigned powerWidths  [0x10000];
  bool hasLoaded        [0x10000];
  unsigned height, powerHeight;
  unsigned rise, dip;
  float mult;

  const char* filename;

  mutable bool hasVAO;
  mutable GLuint vao, vbo;

  public:
  /** Constructs a new Font with the given
   * filename and height. The height is
   * relative to the screen. If stipple is
   * specified, the font is rendered to
   * blend in with a halftone (0x55/0xAA)
   * stipple. The pre and post functions will
   * assume that polygon stippling has
   * been enabled before the call.
   */
  Font(const char* filename, float height, bool stipple=false);
  virtual ~Font();

  /** Returns the width, relative to the screen,
   * of the given character.
   */
  float width(char) const noth;
  /** Returns the width, relative to the screen,
   * of the given string (taking special sequences
   * into account).
   */
  float width(const char*) const noth;

  /** Returns the top-to-bottom height of the font */
  float getHeight() const noth;
  /** Returns the baseline-to-top height of the font */
  float getRise() const noth;
  /** Returns the baseline-to-bottom height of the font */
  float getDip() const noth;

  /** Returns whether the font is stippled */
  bool isStippled() const noth { return stipple; }

  /** Same as the preDraw(float,float,float,float) below, but queries ASGI for the current colour. */
  void preDraw() const noth;

  /** Prepares OpenGL to draw the font. This must be called before draw functions.
   * The colour must be specified here.
   */
  void preDraw(float r, float g, float b, float a=1) const noth;
  /** Restores OpenGL to its pre-draw state. This should be called after draw functions.
   */
  void postDraw() const noth;
  /** Draws the given string at the
   * given coordinate (from left to right). The
   * coordinate is the bottom-left of what is
   * rendered. The text is drawn in the current
   * GL colour. If the specified maximum width is exceeded, drawing
   * stops.
   * Strings may define alternate colours within themselves.
   * This is accomplished via the following syntax (where \a is BEL):
   * \verbatim
   *   \a[XXXXXXXX      Set to hexadecimal RGBA colour XXXXXXXX after
   *                    pushing the current colour
   *   \a[(name)        Set to the named colour name according to the
   *                    setting hud.colours.<name>, after pushing the
   *                    current colour.
   *   \a]              Pop the previous colour. Does nothing if there
   *                    is nothing to pop.
   *   \a_C             Pretend that the next character has the width of
   *                    character C. C MUST be an ASCII character.
   *   \a\a             Pretend that we have encountered the end of
   *                    of the string.
   *   \a&              Has no effect. Reserved for accelerator suggestion.
   *   \a{              Begin blink
   *   \a}              End blink
   * \endverbatim
   * Any leftover push levels are automatically popped at the end of
   * the string as if \a] had been specified.
   *
   * @param str The string to draw; must not be NULL
   * @param x The X coordinate of the bottom-left of the first character
   * @param y The Y coordinate of the bottom-left of the first character
   * @param maxw The maximum width to advance before stopping
   * @param preserveColour If true, colour information will be preserved
   *   across calls to draw (ie, the stack will not be reset).
   * @param fixw If non-NULL, this value will be used for spacing
   *   instead of each character's actual width.
   */
  void draw(const char* str, float x, float y, float maxw=999,
            bool preserveColour = false, const float* fixw = NULL) const noth;
  /** Draws the given character at the given coordinates.
   * This is equivalent to the following:
   * \verbatim
   *   char str[2] = {ch,0};
   *   draw(str,x,y);
   * \endverbatim
   * @param ch The character to draw
   * @param x The X coordinate of the bottom-left of the character
   * @param y The Y coordinate of the bottom-left of the character
   */
  void draw(char ch, float x, float y) const noth;

  private:
  //Used by the constructor so that const Fonts can be created. Besides returning the int height, it
  //also renders the font and sets the widths.
  //Not actually const.
  int renderFont(const char* filename, float virtHeight) const noth;
  //Renders a single character
  void renderCharacter(unsigned ch) noth;
};

/** Standard non-stipped font.
 * This is a pointer to a region of memory that can be used as a Font.
 * This must be manually initialised with the placement-new
 * operator.
 */
extern Font*const sysfont,
           /** Standard stippled font. @see sysfont */
           *const sysfontStipple,
           /** Secondary non-stippled font. @see sysfont */
           *const smallFont,
           /** Secondary stippled font. @see sysfont */
           *const smallFontStipple;

#endif /*FONT_HXX_*/
