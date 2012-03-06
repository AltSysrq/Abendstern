#! /usr/bin/env tclsh
# Jason Lingle
# 2012.02.26
#
# Automatic generator for GameObject networking code.

# After setting up, this prorgam sources definition.tcl, which is to be in
# the same directory as it.
#
# The definition file has the following top-level structure:
#   section {
#     data...
#   }
#   section {
#     data...
#   }
#
# Output is written to xnetobj.hxx and xnetobj.cxx.
#
# Each section describes an object type; there are two types of sections:
#   prototype name declaration
#   type name declaration
# The two are the same, except that prototype does not result in actual code
# (see the "extension" datum). The declaration is eval'ed, which results in
# a list of rules.
# A type results in classes INO_<name> and ENO_<name>, which are subclasses of
# ImportedNetworkObject and ExportedNetworkObject, respectively.
#
# The data (ie, the contents of a section) is used to define the encoding and
# decoding rules. On decoding (ie, parsing binary data), the rules are evaluated
# in the order specified; on encoding, they are evaluated in reverse order.
#
# In the rules below, a PARMS argument is a list alternating between parm name
# and parm argument. "Common parms" refers to the following:
#   encode CODE
#     execute CODE C++ code before encoding; variables:
#       DATA    byte*const, output pointer
#   decode CODE
#     execute CODE C++ code after decoding; variables:
#       DATA    byte*const, input pointer
#   validate CODE
#     execute immediately after decoding
#   extract CODE
#     run this C++ code before encoding, with X as the operating object
#   update CODE
#     run this C++ code after decoding, with X as the operating object, and T
#     as the current latency.
#     May call the macro DESTROY(x) to destroy the object; x is true if
#     the object is being destroyed due to error, false otherwise.
#   post-set CODE
#     Like update, but run after construction. X is the operating object.
#     May NOT set DESTROY (which doesn't exist).
#   declaration CODE
#     use CODE for the declarations
#   compare CODE
#     run to compare between X and Y, which are the local and remote mirror
#     objects. decode and extract have been used to decode to x.NAME and
#     y.NAME, where NAME is the name associated with the datum.
#     The C++ code may modify the variables "near" and "far"; if near exceeds
#     1 after comparison, or far exceeds the distance, it is determined that
#     the object must be updated.
#   inoheader CODE
#     Added to the end of the INO class header (within the class).
#   enoheader CODE
#     Added to the end of the ENO class header (within the clgss).
#   impl CODE
#     Added to the end of the implementation.
#   update-control
#   compare-control
#   transmission-control
#     Used to temporarily enable/disable output (see special rules below).
#     update is for receiving and transmitting updates; compare is during
#     comparisons.
#   set-reference CODE
#     Run after successful construction of an imported object.
#   default MULT
#     If specified, extract, update, and compare are automatically set (if they
#     are not otherwise present) to:
#       extract { NAME = X->NAME; }
#       update  { X->NAME = NAME; }
#       compare {{
#         float delta = fabs(x.NAME-y.NAME);
#         FAR += delta*MULT;
#         NEAR += delta*MULT;
#       }}
# Special rules:
#   If an output entry is encountered whose value is solely a dollar, output
#   is not produced until the next such entry. These entryes themselves are
#   never output.
#
# The following data rules are available:
#   extension SECTION
#     All rules defined in SECTION are copied to the rules list for the current
#     section.
#   ui SZ NAME PARMS
#     Defines an unsigned integer with size in bytes SZ (1,2,3,4,8) bound to
#     variable NAME.
#     PARMS: common parms, minus encode and decode
#   si SZ NAME PARMS
#     Defines a signed integer with size in bytes SZ (1,2,3,4,8) bound to
#     variable NAME.
#     PARMS: common parms, minus encode and decode
#   float NAME PARMS
#     Defines a floating-point bound to variable NAME.
#     PARMS: common parms, minus encode and decode, plus min and max.
#     If no custom validator is given, an automatic one will be generated which
#     ensures that the value is non-NaN and between min and max, inclusive
#     (which default to -1e9 and +1e9, respectively).
#   void PARMS
#     Does nothing but hold PARMS, which are the common parms (minus encode
#     and decode).
#   virtual TYPE NAME PARMS
#     Equivalent to {*}$TYPE $NAME $PARMS, except that the encode and decode
#     parms are left blank (and thus must be filled in manually).
#   bit SZ NAME PARMS
#     Encodes an unsigned integer into SZ bits.
#     PARMS: common parms, plus {type t}, giving the C++ type t to use
#     (defaulting to unsigned).
#   nybble ...
#     Synonym for bit 4 ...
#   str MAXLEN NAME PARMS
#     Defines a NUL-terminated string with maximum length MAXLEN.
#     PARMS: common parms, minus encode and decode.
#     When user code must read from the char NAME[MAXLEN], the last char
#     will always be zero.
#   dat LEN NAME PARMS
#     Defines a byte array of length LEN.
#     PARMS: common parms, minus encode and decode
#   arr CTYPE LEN STRIDE NAME CONTENTS PARMS
#     Defines an array with name NAME, C++-type CTYPE, and length LEN.
#     CONTENTS is evaluated STRIDE times, using varying names to replace "NAME"
#     within. The string "IX" within CONTENTS will be replaced by an expression
#     evaluating to the current datum's index within the array, insensitive of
#     the stride.
#   toggle
#     Synonym for   void { update-control $ compare-control $ }
#   construct CODE
#     Does not give encoding or decoding rules; rather, it provides C++ code
#     to run to assign X to a new instance of the appropriate type. It is not
#     part of the section proper, and is thus not copied by extension.
#
# Additionally, at the top-level, commands
#   verbatimh CODE
#   verbatimc CODE
# is available to write CODE verbatim into the header or implementation output,
# respectively.

file mkdir net/xxx
set hout [open net/xxx/xnetobj.hxx w]
set cout [open net/xxx/xnetobj.cxx w]

puts $hout {
  #ifndef XNETOBJ_HXX_
  #define XNETOBJ_HXX_

  /**
   * @file
   * @author src/net/generate.tcl
   * @brief Autogenerated. Do not edit!
   */

  #include "../object_geraet.hxx"

  /**
   * Creates and returns an ExportedObjectGeraet* that relays the object to the
   * remote peer. The channel is opened automatically.
   *
   * The program is aborted if it is not known how to export the given type of
   * object.
   */
  ExportedGameObject* createObjectExport(NetworkConnection*, GameObject*)
  throw();
}

puts $cout {
  /**
   * @file
   * @author src/net/generate.tcl
   * @brief Autogenerated. Do not edit!
   */

  #include <cstring>

  #include "../io.hxx"

  using namespace std;
  //These diagnostics will happen alot due to the way code is
  //generated; they are safe to ignore.
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  #pragma GCC diagnostic ignored "-Wunused-variable"

  //These are defined to... something on windows
  #ifdef NEAR
  #undef NEAR
  #endif
  #ifdef FAR
  #undef FAR
  #endif

}

# Takes a list of dicts and a key(s); returns a list of entries matching
# that key(s), interleaved.
# Entries that don't exist are omitted.
proc select {from args} {
  set ret [list]
  set active yes
  foreach d $from {
    foreach key $args {
      if {[dict exists $d $key]} {
        if {"\$" == [dict get $d $key]} {
          set active [expr {!$active}]
        } elseif {$active} {
          lappend ret [dict get $d $key]
        }
      }
    }
  }
  return $ret
}

# Selects the given field from the global elements, then joins it with
# newline characters.
proc cxxj {args} {
  join [select $::elements {*}$args] "\n"
}

# Like cxxj, but join in reverse order.
# The keys still reflect the final order, however.
proc jxxc {args} {
  join [lreverse [select $::elements {*}[lreverse $args]]] "\n"
}

set prototypes [dict create]
set classes [list]

proc prototype {name contents} {
  global prototypes
  dict set prototypes $name $contents
}

# Ensures that the current bit offset is zero.
proc whole-byte {} {
  global byteOffset bitOffset
  if {$bitOffset} {
    incr byteOffset
    set bitOffset 0
  }
}

proc type {name contents} {
  global byteOffset bitOffset elements typeConstructor hout cout classes
  # Register the type
  prototype $name $contents
  lappend classes $name
  # Setup for evaluation
  set byteOffset 0
  set bitOffset 0
  set elements [list]
  catch { unset typeConstructor }
  # Evaluate contents
  namespace eval :: $contents
  whole-byte

  # Produce output
  puts $hout \
"class $name;

class INO_$name: public ImportedGameObject {
  NetworkConnection* cxn;
public:
  INO_${name}(NetworkConnection* cxn);
  static const NetworkConnection::geraet_num num;

protected:
  virtual void construct() throw();
  virtual void update() throw();

private:
  $name* decodeConstruct(const std::vector<byte>&) const throw();
  bool decodeUpdate(const std::vector<byte>&, $name*) throw();

  static InputNetworkGeraet* create(NetworkConnection*) throw();

  [cxxj inoheader]
};

class ENO_$name: public ExportedGameObject {
public:
  ENO_${name}(NetworkConnection*, $name*);

protected:
  virtual bool shouldUpdate() const throw();
  virtual void updateRemote() throw();

private:
  void encode() throw();
  $name* clone(const $name*) const throw();

  [cxxj enoheader]
};
"

  puts $cout \
"INO_${name}::INO_${name}(NetworkConnection* cxn_)
: ImportedGameObject($byteOffset, cxn_),
  cxn(cxn_)
{ }

const NetworkConnection::geraet_num INO_${name}::num =
    NetworkConnection::registerGeraetCreator(&create);

InputNetworkGeraet* INO_${name}::create(NetworkConnection* cxn) throw() {
  return new INO_${name}(cxn);
}

void INO_${name}::construct() throw() {
  object = decodeConstruct(state);
}

void INO_${name}::update() throw() {
  if (decodeUpdate(state, static_cast<$name*>(object)))
    destroy(false);
}

$name* INO_${name}::decodeConstruct(const std::vector<byte>& DATA)
const throw() {
  #define DESTROY(x) do { delete X; return NULL; } while(0)
  const unsigned T = cxn->getLatency();
  [cxxj declaration]
  [cxxj decode validate]
  $name* X;
  $typeConstructor
  [cxxj post-set]
  [cxxj set-reference]
  return X;
  #undef DESTROY
}

bool INO_${name}::decodeUpdate(const std::vector<byte>& DATA, $name* X)
throw () {
  #define DESTROY(x) return true
  const unsigned T = cxn->getLatency();
  [cxxj update-control declaration]
  [cxxj update-control decode validate update]
  return false;
  #undef DESTROY
}

ENO_${name}::ENO_${name}(NetworkConnection* cxn, $name* obj)
: ExportedGameObject($byteOffset, cxn, obj, clone(obj))
{
  //Populate initial data
  #define X obj
  #define field (cxn->field)
  const unsigned T = cxn->getLatency();
  #define DATA state
  [cxxj declaration]
  [jxxc extract encode]
  #undef DATA
  #undef field
  #undef X
}

$name* ENO_${name}::clone(const $name* src) const throw() {
  #define X src
  #define field (&this->cxn->field)
  #define DESTROY(x) assert(!(x))
  const unsigned T = cxn->getLatency();
  [cxxj declaration]
  [jxxc extract]
  #undef X
  $name* dst;
  #define X dst
  $typeConstructor
  [cxxj post-set]
  #undef X
  #undef field
  #undef DESTROY
  return dst;
}

bool ENO_${name}::shouldUpdate() const throw() {
  float NEAR = 0, FAR = 0;
  struct S {
    [cxxj compare-control declaration]
    S(const $name* X) {
      [cxxj compare-control extract]
    }
  } x(static_cast<$name*>(local.ref)), y(static_cast<$name*>(remote));

  [cxxj compare-control compare]

  float l_dist = cxn->distanceOf(this->local.ref);
  return (FAR > l_dist) || (NEAR > 1 && l_dist < 5);
}

void ENO_${name}::updateRemote() throw() {
  #define T 0
  #define DATA (this->state)
  #define field (&this->cxn->field)
  #define DESTROY(x) assert(!(x))
  $name* l_local = static_cast<$name*>(this->local.ref);
  $name* l_remote = static_cast<$name*>(this->remote);
  [cxxj update-control declaration]
  #define X l_local
  [jxxc update-control extract encode]
  #undef X
  #define X l_remote
  [cxxj update-control update]
  #undef X
  #undef DATA
  #undef T
  #undef DESTROY
  #undef field
}
[cxxj impl]
"
}

# Expands to DATA[$::byteOffset+$off]
proc data {{off 0}} {
  return "DATA\[$::byteOffset+$off\]"
}

# Sets the global current to an empty dictionary.
proc new {} {
  global current
  set current [dict create]
}

# Appends current to the global elements
proc save {} {
  global current elements
  lappend elements $current
}

# Adds aliasing rules to the global current for the given name.
proc aliases {name} {
  global current
}

# Returns the C++ type to use for an integer of the given size.
proc int-type-for {signed sz} {
  if {$signed} {
    set s signed
  } else {
    set s unsigned
  }
  switch $sz {
    1 {return "$s char"}
    2 {return "$s short"}
    3 {return "$s int"}
    4 {return "$s int"}
    8 {return "$s long long"}
  }
  error "Invalid integer size $sz"
}

# Adds a type declaration for the given integer type and name.
proc int-decl {signed sz name} {
  dict set ::current declaration "[int-type-for $signed $sz] $name;"
}

# Adds encoding information for the given integer type and name
proc int-enc {signed sz name} {
  global current byteOffset
  switch $sz {
    3 {
      dict set current encode "io::write24_c(&[data], $name);"
      dict set current decode "io::read24_c(&[data], $name);"
    }
    default {
      dict set current encode "io::write_c(&[data], $name);"
      dict set current decode "io::read_c(&[data], $name);"
    }
  }
}

# Adds the given parms to current
proc eval-parms {parms {name {}}} {
  global current
  foreach {k v} $parms {
    dict set current $k $v
  }

  if {[dict exists $current default]} {
    set mult [dict get $current default]
    if {![dict exists $current update]} {
      dict set current update "X->$name = $name;"
    }
    if {![dict exists $current extract]} {
      dict set current extract "$name = X->$name;"
    }
    if {![dict exists $current compare]} {
      dict set current comare \
      "{float d=fabs(x.$name-y.$name)*$mult;FAR+=d;NEAR+=d;}"
    }
  }
}

proc extension section {
  global prototypes
  namespace eval :: [dict get $prototypes $section]
}

proc construct code {
  set ::typeConstructor $code
}

proc elt-integer {sign sz name parms} {
  global byteOffset

  whole-byte
  new
  aliases $name
  int-decl $sign $sz $name
  int-enc $sign $sz $name
  eval-parms $parms $name
  incr byteOffset $sz
}

proc ui {sz name {parms {}} {save yes}} {
  elt-integer no $sz $name $parms
  if {$save} save
}
proc si {sz name {parms {}} {save yes}} {
  elt-integer yes $sz $name $parms
  if {$save} save
}
proc float {name {parms {}} {save yes}} {
  global current byteOffset

  whole-byte
  new
  aliases $name
  dict set current declaration "float $name;"
  dict set current encode "io::write_c(&[data], $name);"
  dict set current decode "io::read_c(&[data], $name);"
  dict set current min -1.0e9
  dict set current max +1.0e9
  eval-parms $parms $name
  if {![dict exists $current validate]} {
    dict set current validate \
"if ($name != $name) $name = [dict get $current min];
 else if ($name < [dict get $current min]) $name = [dict get $current min];
 else if ($name > [dict get $current max]) $name = [dict get $current max];"
  }
  incr byteOffset 4
  if {$save} save
}

proc void {parms} {
  new
  eval-parms $parms
  save
}

proc arr {ctype len stride name contents {parms {}} {save yes}} {
  global current byteOffset bitOffset elements

  if {$len%$stride} {
    error "Array length must be a multiple of its stride."
  }

  whole-byte
  set oldByteOffset $byteOffset
  set byteOffset 0

  set oldElements $elements
  set elements [list]
  # Populate the contents
  for {set i 0} {$i < $stride} {incr i} {
    eval [string map [list NAME "$name\[$i+ARRAY_OFFSET\]" \
                           IX "($i+ARRAY_OFFSET)"] $contents]
  }
  whole-byte
  set strideLength $byteOffset
  set byteOffset $oldByteOffset
  set arrayLength [expr {$strideLength*$len/$stride}]

  # Replace DATA in the elements with (DATA+OFF+ARRAY_OFFSET*STRIDE)
  set elements \
    [string map \
            [list DATA "(&DATA\[0\]+$byteOffset+ARRAY_OFFSET*$strideLength)"] \
            $elements]

  set loop \
  "for (unsigned ARRAY_OFFSET=0; ARRAY_OFFSET<$len; ARRAY_OFFSET+=$stride)"

  # Set array up
  whole-byte
  new
  aliases $name
  dict set current declaration "$ctype $name\[$len\];"
  if {{} ne [cxxj encode]} {
    dict set current encode "$loop {\n[cxxj encode]\n}"
  }
  if {{} ne [cxxj decode]} {
    dict set current decode "$loop {\n[cxxj decode]\n}"
  }
  if {{} ne [cxxj validate]} {
    dict set current validate "$loop {\n[cxxj validate]\n}"
  }
  if {{} ne [cxxj extract]} {
    dict set current extract "$loop {\n[cxxj extract]\n}"
  }
  if {{} ne [cxxj update]} {
    dict set current update "$loop {\n[cxxj update]\n}"
  }
  if {{} ne [cxxj post-set]} {
    dict set current post-set "$loop {\n[cxxj post-set]\n}"
  }
  if {{} ne [cxxj compare]} {
    dict set current compare "$loop {\n[cxxj compare]\n}"
  }
  eval-parms $parms
  incr byteOffset $arrayLength

  # Restore old elements, then possibly save
  set elements $oldElements
  if {$save} save
}

proc virtual args {
  global byteOffset current
  set bo $byteOffset
  # Evaluate without saving
  {*}$args no
  dict unset current encode
  dict unset current decode
  set byteOffset $bo
  save
}

proc bit {sz name {parms {}} {save yes}} {
  global bitOffset byteOffset current
  if {$bitOffset+$sz > 8} {
    # Move to next byte
    whole-byte
  }
  set shift $bitOffset
  incr bitOffset $sz
  # Mask after shifting
  set mask [expr {(1 << $sz)-1}]

  new
  aliases $name
  dict set current declaration "unsigned $name;"
  dict set current decode "$name = ([data] >> $shift) & $mask;"
  # In encoding, first zero out the bits we use, then write to them.
  dict set current encode \
  "[data] &= ~($mask<<$shift); [data] |= ($name & $mask) << $shift;"
  eval-parms $parms $name

  # If a type parm was given, replace the declaration
  if {[dict exists $current type]} {
    dict set current declaration "[dict get $current type] $name;"
  }

  if {$save} save
}

proc nybble {args} {
  bit 4 {*}$args
}

proc str {maxlen name {parms {}} {save yes}} {
  global byteOffset current

  whole-byte
  new
  aliases $name
  dict set current declaration "char $name\[$maxlen\];"
  dict set current decode \
  "strncpy($name, (const char*)&[data], $maxlen-1); $name\[$maxlen-1]=0;"
  dict set current encode \
  "strncpy((char*)&[data], $name, $maxlen);"
  eval-parms $parms $name
  incr byteOffset $maxlen

  if {$save} save
}

proc dat {len name {parms {}} {save yes}} {
  global byteOffset current

  whole-byte
  new
  aliases $name
  dict set current declaration "byte $name\[$len\];"
  dict set current decode "memcpy($name, &[data], $len);"
  dict set current encode "memcpy(&[data], $name, $len);"
  eval-parms $parms $name
  incr byteOffset $len

  if {$save} save
}

proc toggle {} {
  void { update-control $ compare-control $ }
}

proc verbatimh {code} {
  puts $::hout $code
}

proc verbatimc {code} {
  puts $::cout $code
}

source net/definition.tcl

# Write the exporter creator
puts $cout "
  #include \"../synchronous_control_geraet.hxx\"
  ExportedGameObject* createObjectExport(NetworkConnection* cxn,
                                         GameObject* object)
  throw() {
    ExportedGameObject* ego;
"
foreach class $classes {
  puts $cout "if (typeid(*object) == typeid($class))"
  puts $cout "  ego = new ENO_${class}(cxn, ($class*)object);"
  puts $cout "else"
}

puts $cout "
  {
    cerr << \"FATAL: Unknown object type to export: \"
         << typeid(*object).name() << endl;
    assert(false);
    exit(EXIT_PROGRAM_BUG);
  }

  assert(ego);
  cxn->scg->openChannel(ego, INO_${class}::num);
  return ego;
}"

puts $hout "#endif"
close $hout
close $cout
