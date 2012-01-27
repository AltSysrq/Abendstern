/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AObject base class.
 */
#ifndef AOBJECT_HXX_
#define AOBJECT_HXX_

#include "src/opto_flags.hxx"

struct Tcl_Interp;

/** AObject is the common superclass of almost all Abendstern objects.
 *
 * It exists
 * not only to force a virtual destructor and provide a common superclass, but also
 * to contain Tcl memory ownership information.
 *
 * Its data members are public, but should not be used by code outside of the auto-
 * generated C++-Tcl glue code.
 */
class AObject {
  public:
  /**\cond INTERNAL*/
  //Tcl memory information; include backup so
  //ownership change can be undone in case of error
  enum OwnershipStatus { Cpp, Tcl, Container } ownStat, ownStatBak;
  Tcl_Interp* tclExtended; //Set if this is a Tcl-extended type
  union {
    Tcl_Interp* interpreter;
    void* container;
  } owner, ownerBak;
  //Set to true if any Tcl reference to this object was ever made
  mutable bool tclKnown;

  /**\endcond*/

  //AObject() : ownStat(Cpp), tclExtended(NULL) {}
  //../aobject.hxx:32: error: 'NULL' was not declared in this scope
  //WTF?
  /** Constructs the AObject and properly sets internal information. */
  AObject() : ownStat(Cpp), tclExtended(0), tclKnown(false) {}

  //Implemented in tcl_iface/implementation.hxx to free appropriate
  //information
  virtual ~AObject();
};
#endif /*AOBJECT_HXX_*/
