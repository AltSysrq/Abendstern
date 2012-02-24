This folder contains the development files used to produce a working interface
between C++ and Tcl (using Incr Tcl for objects). Unlike boost::python and the
equivilant Tcl library, this interface is defined in Tcl and uses another Tcl
script to autogenerate C++- and Tcl-side bindings. Also unlike those two, it
supports a workable memory model that allows seamless integration of the two
languages (versus nothing but segfaults).

Instead of trying to directly bind C++ to Incr Tcl, this interface utilizes two
intermediate layers, meaning the whole communication sequence is
  C++ <==> C++ Wrapper <==> (Tcl Interpreter) <==> Tcl Wrapper <==> Incr Tcl

The C++ wrapper is used to forward virtual function calls between C++ and Tcl,
to allocate C++ objects with new, and to provide functions to get and set
procedures for member variables. The interface exposed directly from the C++
Wrapper to Tcl is not really object-oriented, and is far from being Incr Tcl.
The Tcl Wrapper, therefore, uses Incr Tcl to wrap this interface into something
more useful.

One significant difference between the C++ objects and normal Incr Tcl objects
is the use of constructors. Instead of using
  <class> <name> ?args?
one MUST call
  new <class> ?args?
to create a new object. The new proc first gets a C++-recognized name for the
object, allocates the C++ object, then calls the normal constructor. If the new
proc is not used, the result will be a Tcl-only object with an invalid pointer
to the C++ class it is extending.

The memory model divides objects into three categories:
+ native Abendstern objects
+ Tcl-extended Abendstern objects
+ foreign (non-Abendstern) objects

For native Abendstern objects, the interface keeps track of three things:
+ Whether Tcl has a reference to the object (and if so, what it is)
+ The owner (Tcl or C++) of the object
These properties are used to regulate and catch errors in object deletion. When
C++ deletes a native object, the following checks are done:
+ If Tcl is the current owner, the program aborts, as the interface has been
  misconfigured
+ If Tcl has a reference, that reference is deleted
When Tcl deletes a native abendstern object:
+ If Tcl is the current owner, the C++ object is deleted;
+ either way, the Tcl reference is deleted.

Tcl-extended Abendstern objects store the the same data as native Abendstern
objects; however, Tcl must ALWAYS have a reference to the object, as that
contains the Tcl extensions. The C++ deletion checks are identical; for Tcl:
+ If Tcl is the current owner, both C++ and Tcl object are deleted
+ If C++ is the current owner, the script has performed a fatal error

Foreign objects can be neither created nor destroyed by Tcl.

Passing arguments to functions, returning values from functions, and changing
the value of C++-side variables that are native C++ objects (or Tcl-extended)
can all affect the ownership of objects. These effects are:
+ Stealing Ownership
  When applied to a function argument, the system (C++ or Tcl) that receives it
  gains control of the object. Applied to a function return value, the system
  that called the function gains control of the object. For variables, C++
  (always, since Tcl variables don't interface) gains control of the new value,
  and the calling code (always Tcl, since nothing happens for C++) gets control
  of the old value. If Tcl does not currently have a reference to the old value,
  it is automatically deleted.
+ Strictly Stealing Ownership
  Same as Stealing Ownership, but it is an error for this to transfer ownership
  to C++ if it is already there (an example is GameField::add).
+ Yielding Ownership
  Only applied to function arguments, it indicates that ownership transfers to
  the system the /function/ is running in.
+ Strictly Yielding Ownership
  Same as Yielding Ownership, but ownership MUST be C++ at the time of calling
  (example is GameField::remove).
+ Borrowing
  No transfer of ownership is incurred.

INTERFACE DESCRIPTION
Names: Names are either the plain C++ name (which then becomes the Tcl name as
well), or a list containing the C++ name and the Tcl name
(ie, {operator[] elt}).

Types: All types are specified according to their C++ name. Pointers/references
must be included so that code generation can handle immediate values correctly,
and, of course, to specify the C++ types in the glue code correctly. Some C++
types are adapted to be compatible via a special template class in Abendstern;
therefore, the type ao<...> is recognized appropriately. Other template types
can be declared.
Top level:
  enum name prefix value0 value1 ...
    Defines an enumeration of constants. The name is the name of the enumeration
    type. prefix is automatically prepended to each of the values. values is a
    list of names for the various value in the enumeration; these are normal
    strings in Tcl, but are automatically converted upon use. Values function as
    normal identifiers, supporting an alternate name for the Tcl side.
  const name type ?typemods?
    Defines a constant with the given name and type. If an object, it will
    always belong to C++.
  var name type ?modifiers?
    Defines a global variable with the given name and type. Modifiers may be
    present to describe how modifications work with the memory model.
  fun name return-type-and-return-type-modifiers ?arg0? ?arg1? ?arg2? ...
    Defines a global function. Return type works as expected. Each argument is a
    list beginning with the type of the argument, and followed by modifiers.
    Tcl-side functions canNOT be overloaded, so renaming may be necessary.
  class {abstract-extendable|extendable|final|foreign} name superclasses ?body?
    Defines a new class. If the class is extendable, it is a native Abendstern
    class that can be extended by Tcl; if final, a native Abendstern class that
    may NOT be extended by Tcl; foreign simply dictates that it is not an
    Abendstern class. Superclasses lists the classes that this class extends; if
    empty, defaults to AObject for extendable and final classes, and to nothing
    for foreign classes. The body consists of a list of const, var, and fun
    declarations (a modified member form). The body can be omitted for a class
    predeclaration in case there are codependant.
  template basename args body
    Defines a template for a type. This is implemented as a proc named
    generate_basename which is passed the given arguments, and then produces one
    or more class definitions; for example:
      template vector T {class final vector<$T> {...}}
Class context:
  const name type member-modifiers type-modifiers
    Same as normal, except for member-modifiers.
  var name type member-modifiers type-modifiers
    Same
  fun name return-type-and-return-type-modifiers member-modifiers ?args?
    Mostly the same as the normal function declaration, except for the
    member modifiers.
  constructor constructor-name ?arg0? ?arg1? ?arg2? ...
    Mostly the same as a function. Since many classes have multiple
    constructors, each is assigned a unique name to allow C++ to choose the
    correct one.
Type modifiers:
  steal   Specifies Stealing Ownership.
  STEAL   Specifies Strictly Stealing Ownership.
  yield   Specifies Yielding Ownership.
  YIELD   Specifies Strictly Yielding Ownership.
  copy    Indicates that the value should be copied into a destination-owned
          object first (this excludes the other memory model modifiers).
  {check} "check" is followed by a segment of C++ code that determines whether
          the value is acceptable. In all cases, the code has a variable "val"
          of the /original/ type (or a reference thereto) and sets a boolean
          "ok" (which defaults to true).
          The meaning varies slightly by context:
            variable: val is the new value for the variable. Called before the
                      variable is modified. "currVal" is also available as the
                      current variable value.
            argument: val is the argument. Called upon function entering C++
                      function; multiple verifiers are run left to right.
            return:   val is the value returned from a Tcl override, after being
                      converted back to the true return type.
          The class context (whether in a class, and whether static) correspond
          to the current item.
  const   The item is const. This is only used for the C++ glue code.
  throw * C++ throw specifier
  noth    Similar to throw, but uses the noth macro.
Member modifiers:
  public
  protected Same as Tcl modifiers. These have absolutely no bearing on the C++
            specification.
  static    The member is static to the class.
  virtual   The function is virtual. This must only be specified within
            extendable classes.
  purevirtual  Same as virtual, but there is no default.
  const     The member function is const
Other things:
  safe-only {...}, unsafe {...}
    The contained blocks will only be declared for safe (former) or
    trusted (latter) code.
  cxx ?args?
    Rotates to the next implementation output file, #include'ing each arg given.

SPECIAL TYPES
The following types are recognized and handled specially:
  int unsigned short ushort uint32 float double bool char
  cstr (const char*) string void tclobj

GENERATION
The generation script produces the following files (relative to Abendstern's
root):
  tcl_iface/bridge.cxx          All definitions, as well as
                                generateInterpreter(bool safe)
  tcl_iface/xxx/xX.cxx          (X=0..f) implementation fragments
  tcl/bridge.tcl                Bridge to Incr Tcl
The cxx file depends on the header "bridge.hxx" and "implementation.hxx" in the
same directory.

The bridge.cxx file contains only the entry point; ie, the function that calls
all other global-level functions.
