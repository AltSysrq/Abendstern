#! /usr/bin/env tclsh
# Jason Lingle
# 2012.02.26
#
# Automatic generator for GameObject networking code.
package require Itcl

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
#       DST     byte*const, output pointer
#   decode CODE
#     execute CODE C++ code after decoding; variables:
#       SRC     byte*const, input pointer
#   extract CODE
#     run this C++ code before encoding, with X as the operating object
#   update CODE
#     run this C++ code after decoding, with X as the operating object, and T
#     as the current latency.
#   destroy CODE
#     run this C++ code before encoding destruction information
#   nodeclare {}
#     suppress declaration of variable associated
#   compare CODE
#     run to compare between X and Y, which are the local and remote mirror
#     objects. decode and extract have been used to decode to x_NAME and
#     y_NAME, where NAME is the name associated with the datum.
#     The C++ code may modify the variables "near" and "far"; if near exceeds
#     1 after comparison, or far exceeds the distance, it is determined that
#     the object must be updated.
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
#     PARMS: common parms, minus encode and decode
#   array TYPE SZ NAME PARMS
#     Equivalent to
#       {*}$TYPE $NAME[0] {nodeclare {}}
#       {*}$TYPE $NAME[1] {nodeclare {}}
#       ...
#       {*}$TYPE $NAME[$SZ-1] {nodeclare {}}
#     except that the array is declared correctly, and the parms are applied
#     once at the end (except for compare, which is propagated to the
#     children).
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
#   string MAXLEN NAME PARMS
#     Defines a NUL-terminated string with maximum length MAXLEN.
#     PARMS: common parms, minus encode and decode.
#     When user code must read from the char NAME[MAXLEN], the last char
#     will always be zero.
#   data LEN NAME PARMS
#     Defines a byte array of length LEN.
#     PARMS: common parms, minus encode and decode
#   constructor CODE
#     Does not give encoding or decoding rules; rather, it provides C++ code
#     to run to assign X to a new instance of the appropriate type. It is not
#     part of the section proper, and is thus not copied by extension.
#
# Additionally, at the top-level, commands
#   verbatimh CODE
#   verbatimc CODE
# is available to write CODE verbatim into the header or implementation output,
# respectively.
