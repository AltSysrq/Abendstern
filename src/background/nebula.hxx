/**
 * @file
 * @author Jason Lingle
 *
 * @brief Contains the Nebula class as well as types for working with
 * its equations.
 */

#ifndef NEBULA_HXX_
#define NEBULA_HXX_

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>
#include <cfloat>

#include <GL/gl.h>
#include <SDL.h>
#include <SDL_thread.h>

#include "background.hxx"

class GameObject;
class GameField;

/** The NebulaResistanceElement contains vectors for each point of an
 * object. These describe absolute location, velocity, and the vector
 * to the centre of gravity for the object (used for calculating
 * torsion).
 */
struct NebulaResistanceElement {
  float
    /** The X coordinate of the element */
    px,
    /** The Y coordinate of the element */
    py,
    /** The X velocity of the element */
    vx,
    /** The Y velocity of the element */
    vy,
    /** The X component of the element-to-centre-of-gravity vector */
    cx,
    /** The Y component of the element-to-centre-of-gravity vector */
    cy;
};

/** Contains classes and structures for defining Nebula equations. */
namespace nebula_equation {
  /** This is the base class for equation elements.
   *
   * The Nebula allows a pair of equations to describe the natural flow
   * of gas at any point. Each of these elements represents a single
   * operation. These equations are also used to describe natural pressure.
   */
  class Element {
    public:
    /** Evaluates the element at the given coordinate pair,
     * and returns the relult. Sub-elements must receive
     * the same pair.
     *
     * @param x The X coordinate to evaluate
     * @param y The Y coordinate to evaluate
     * @return The result of evaluation
     */
    virtual float eval(float x,float y) const = 0;

    virtual ~Element() {}

    private:
    bool operator=(const Element&) const; //NOT IMPLEMENTED
  };
  /** Common class for functions that take one argument.
   *
   * @param f The function that performs the actual operation
   */
  template<float (*f)(float)>
  class UnaryElement: public Element {
    std::auto_ptr<const Element> sub;

    public:
    /** Constructs a new UnaryElement of this function.
     *
     * @param s The Element to evaluate to get the input value for
     *   the function f. The UnaryElement gains control of this
     *   Element.
     */
    UnaryElement(const Element* s)
    : sub(s) {}
    virtual float eval(float x, float y) const {
      float r = f(sub->eval(x,y));
      if (r < FLT_MIN || r > FLT_MAX || r != r) r=0;
      return r;
    }
  };
  /** Common class for functions that take two arguments.
   *
   * @param f The function that performs the actual operation
   */
  template<float (*f)(float,float)>
  class BinaryElement: public Element {
    std::auto_ptr<const Element> l,r;

    public:
    /** Constructs a new BinaryElement of this function.
     *
     * @param sl The Element to evaluate to get the first input value
     *   for the function f. The BinaryElement gains control of this Element.
     * @param sr The Element to evaluate to get the second input value
     *   for the function f. The BinaryElement gains control of this Element.
     */
    BinaryElement(const Element* sl, const Element* sr)
    : l(sl), r(sr) {}
    virtual float eval(float x, float y) const {
      float rt = f(l->eval(x,y), r->eval(x,y));
      if (rt < FLT_MIN || rt > FLT_MAX || rt != rt) rt=0;
      return rt;
    }
  };

  /** Simple Element that resolves to a constant value.
   *
   * Notation: Any token beginning with a digit
   */
  class Constant: public Element {
    float v;

    public:
    /** Constructs a new constant with the given value.
     *
     * @param f The value to return from eval()
     */
    Constant(float f) : v(f) {}
    /** Returns the constructed constant.
     *
     * @param x ignored
     * @param y ignored
     * @return v
     */
    virtual float eval(float x,float y) const { return v; }
  };

  /**  Simple Element that resolves to the X coordinate.
   *
   * Notation: x
   */
  class X: public Element {
    public:
    /** Returns the X coordinate.
     *
     * @param x X coordinate
     * @param y ignored
     * @return x
     */
    virtual float eval(float x,float y) const { return x; }
  };
  /** Simple Element that resolves to the Y coordinate.
   *
   * Notation: y
   */
  class Y: public Element {
    public:
    /** Returns the Y coordinate.
     *
     * @param x ignored
     * @param y Y coordinate
     * @return y
     */
    virtual float eval(float x,float y) const { return y; }
  };

  /** Conditional Element.
   *
   * Evaluates its first sub-Element; if it returns greater-than
   * or equal-to zero, the second sub-Element is evaluated and
   * the result returned; otherwise, the third sub-Element is
   * evaluated and returned.
   *
   * Notation: ?
   */
  class If: public Element {
    std::auto_ptr<const Element> cond, then, esle;
    public:
    /** Constructs a new Conditional with the specified children.
     *
     * @param a The Element to test. The If gains control of this Element.
     * @param b The Element to evaluate if a returns >= 0. The If gains control of this Element.
     * @param c The Element to evaluate if a returns < 0. The If gains control of this Element.
     */
    If(const Element* a, const Element* b, const Element* c)
    : cond(a), then(b), esle(c) {}
    virtual float eval(float x, float y) const {
      if (cond->eval(x,y) >= 0)
        return then->eval(x,y);
      else
        return esle->eval(x,y);
    }
  };

  //These are templates because they can't be static (apparently static
  //functions do not make valid template arguments), and they can't be
  //extern either, because then they are multiply-defined.
  #define UNA(op,nam,tnam) template<typename T> T nam(float x) { return op x; } typedef UnaryElement<nam<float> > tnam
  #define BIN(op,nam,tnam) template<typename T> T nam(float x, float y) { return x op y; } typedef BinaryElement<nam<float> > tnam
  /** Addition. Notation: + */
  BIN(+,plus, Add);     //Notation: +
  /** Subtraction. Notation: - */
  BIN(-,minus,Sub);     //Notation: -
  /** Multiplication. Notation: * */
  BIN(*,times,Mul);     //Notation: *
  /** Division. Notation: / */
  BIN(/,divid,Div);     //Notation: /
  /** Exponentiation. Notation: ^, ** */
  typedef BinaryElement<std::pow> Pow;  //Notation: ^, **
  /** Modulus. Notation: % */
  typedef BinaryElement<std::fmod> Mod; //Notation: %
  /** Negation. Notation: _ */
  UNA(-,negat,Neg);     //Notation: _
  #undef UNA
  #undef BIN
  /** Cosine function. Notation: cos */
  typedef UnaryElement<std::cos> Cos;
  /** Sine function. Notation: sin */
  typedef UnaryElement<std::sin> Sin;
  /** Tangent function. Notation: tan */
  typedef UnaryElement<std::tan> Tan;
  /** Hyberbolic cosine function. Notation: cosh */
  typedef UnaryElement<std::cosh> Cosh;
  /** Hyperbolic sine function. Notation: sinh */
  typedef UnaryElement<std::sinh> Sinh;
  /** Hyperbolic tangent function. Notation: tanh */
  typedef UnaryElement<std::tanh> Tanh;
  /** Arc-cosine function. Notation: acos */
  typedef UnaryElement<std::acos> Acos;
  /** Arc-sine function. Notation: asin */
  typedef UnaryElement<std::asin> Asin;
  /** Arc-tangent function, single-argument. Notation: atan */
  typedef UnaryElement<std::atan> Atan;
  /** Arc-tangent function, dual-argument. Notation: atan2 */
  typedef BinaryElement<std::atan2> Atan2;
  /** Square-root function. Notation: sqrt */
  typedef UnaryElement<std::sqrt> Sqrt;
  /** Absolute value. Notation: abs, |, || */
  typedef UnaryElement<std::fabs> Abs;
  /** Exponent-of-e. Notation: e^, e** */
  typedef UnaryElement<std::exp> Exp; //Notation: e^, e**
  /** Natural logarithm. Notation: ln */
  typedef UnaryElement<std::log> Ln; //Notation: ln
  /** Base-10 logarithm. Notation: log */
  typedef UnaryElement<std::log10> Log; //Notation: log
  /** Ceiling function. Notation: |^ */
  typedef UnaryElement<std::ceil> Ceil; //Notation: ceil, |^
  /** Floor function. Notation: |_ */
  typedef UnaryElement<std::floor> Floor; //Notation: floor, |_
  //Apparently, on MSVC++2010, min and max are macros, so std::max etc is invalid
  #ifndef WIN32
  template<typename T> T fmin(float a, float b) { return std::min(a,b); }
  template<typename T> T fmax(float a, float b) { return std::max(a,b); }
  #else
  template<typename T> T fmin(float a, float b) { return min(a,b); }
  template<typename T> T fmax(float a, float b) { return max(a,b); }
  #endif
  /** Minimum function. Notation: min */
  typedef BinaryElement<fmin<float> > Min; //Notation: min
  /** Maximum function. Notation: max */
  typedef BinaryElement<fmax<float> > Max; //Notation: max
};

/** The Nebula is a background which interacts with physical objects,
 * such as ships. Nebula interaction is divided into two categories,
 * based on distance.
 * An object more than 8 screens away from the centre (the limit of
 * simulation distance) acts as if it were in free space.
 *
 * Objects within the simulation area are simulated based on interactions
 * of their NebulaResistanceElements and representations of gaseous
 * flow.
 *
 * The nebula performs simulation on a separate thread, and relies on
 * being drawn and updated before the respective operations on the
 * game field. It also depends on a delay of at least one physical frame
 * between the removal and deletion of objects.
 * (The multithreading is not currently the case, since the OpenGL state
 * cannot be shared between the threads).
 */
class Nebula: public Background {
  static const unsigned numSimScreens = 8;
  static const unsigned pointsPerScreen = 128;
  static const unsigned simSideSz = pointsPerScreen*numSimScreens;
  static const unsigned naturalGridSz = 1024;

  const nebula_equation::Element* baseFlowX, *baseFlowY,
                                * basePressure;
  typedef struct { float p,vx,vy; } points_t[simSideSz*simSideSz];
  points_t points0, points1, *pointsFront, *pointsBack;

  /* Keep track of the most recent bottom-left coordinate of the
   * simulation, in internal points. World coordinates are simply
   * this over (float)pointsPerScreen.
   */
  signed pointsBLX, pointsBLY;

  float colourR, colourG, colourB;
  float glareR, glareG, glareB;

  /* Viscosity, in mass/screens/ms (μ). */
  float viscosity;
  /* Density of the gass; specifically, the number of mass units
   * of one point.
   */
  float density;
  /* The "rubber" of the pressure and velocities. */
  float prubber, vrubber;
  /* Arbitrary constant to multiply force by */
  float forceMul;

  GameObject* reference;
  GameField* field;

  SDL_sem* updateLock, *gpuLock;
  //These two refer to the main thread
  bool hasUpdateLock, hasGPULock;
  SDL_Thread* thread;
  volatile float lastUpdateTime;
  float updateTimeAccum;
  volatile bool alive;

  struct ObjectInfo { GameObject* obj; const NebulaResistanceElement* elts; unsigned len; };
  std::vector<ObjectInfo> objectInfo;

  struct buffer {
    GLuint fbo, tex; //R=pressure, G=velx, B=vely
  } buffer0, buffer1, *frontBuffer, *backBuffer;
  //These cover the entire field
  float naturals[naturalGridSz*naturalGridSz*3]; //pressure, velx, vely
  GLuint naturalsTex; //R=pressure, G=velx, B=vely

  GLuint feedbackBuffer;

  public:
  /** Creates a new Nebula with the specified parameters.
   * The equations are initialised to 0 base flow and
   * 1 base pressure.
   *
   * @param o Initial reference; may be NULL
   * @param f The field this will be a background to
   * @param r The red component of the base colour
   * @param g The green component of the base colour
   * @param b The blue component of the base colour
   * @param viscosity The viscosity of the fluid
   * @param density The mass-density of the fluid
   */
  Nebula(GameObject* o, GameField* f, float r, float g, float b, float viscosity, float density);
  virtual ~Nebula();

  /** Set the flow equations. The const char* versions split
   * the input string on spaces and parse each token as described
   * alongside the equation elements above. This uses prefix notation
   * without parentheses (eg,
   * (-b+sqrt(b^2-4*a*c))/(2*a) = / + _ b sqrt - ^ b 2 * * 4 a c * 2 a).
   * However, parentheses may be used, and will be replaced with spaces in preprocessing,
   * so the above could have been written:
   *   / (+ _ b sqrt(- (^ b 2) (* * 4 a c))) (* 2 a)
   * Returns NULL on success, error description otherwise. If the reset
   * argument is false, the state is not reset to the values
   * indicated by the equations.
   *
   * The versions that take a nebula_equation::Element gain sole ownership of
   * the object.
   *
   * If the last argument is false, the current values will NOT be altered.
   *
   * @return NULL on success, error message otherwise
   */
  const char* setFlowEquation(const nebula_equation::Element*, const nebula_equation::Element*, bool=true);
  /** Sets the flow equations.
   *
   * @see setFlowEquation(const nebula_equation::Element*,const nebula_equation::Element*, bool)
   * @return NULL on success, error message otherwise
   */
  const char* setFlowEquation(const char*, const char*, bool=true);
  /** Sets the pressure equation.
   *
   * @see setFlowEquation(const nebula_equation::Element*,const nebula_equation::Element*, bool)
   * @return NULL on success, error message otherwise
   */
  const char* setPressureEquation(const nebula_equation::Element*, bool=true);
  /** Sets the pressure equation.
   *
   * @see setFlowEquation(const nebula_equation::Element*,const nebula_equation::Element*, bool)
   * @return NULL on success, error message otherwise
   */
  const char* setPressureEquation(const char*, bool=true);

  /** Sets the time required, in milliseconds, for the pressure at
   * a given point to revert to its natural value.
   * Default is 10000.
   */
  void setPressureResetTime(float) noth;
  /** Returns the time required, in milliseconds, for the pressure
   * at a given point to revert to its natural value.
   */
  float getPressureResetTime() const noth;
  /** Sets the time required, in milliseconds, for the velocity at
   * a given point to revert to its natural value.
   * Default is 10000.
   */
  void setVelocityResetTime(float) noth;
  /** Returns the time required, in milliseconds, for the pressure
   * at a given point to revert to its natural value.
   */
  float getVelocityResetTime() const noth;

  /** Sets the constant by which to multiply force exerted on objects.
   * Default is 1.
   */
  void setForceMultiplier(float f) noth { forceMul = f; }
  /** Returns the constant by which to multiply force exerted on objects. */
  float getForceMultiplier() const noth { return forceMul; }

  virtual void draw() noth;
  virtual void postDraw() noth;
  virtual void update(float) noth;
  virtual void updateReference(GameObject*, bool) noth;
  virtual void explode(Explosion*) noth;

  /** Used internally */
  static void attachFeedback(GLuint);

  private:
  void runOnce();
  void run_impl();
  static int run(void*);

  void blitTextures() noth;
};
#endif /* NEBULA_HXX_ */
