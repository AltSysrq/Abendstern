/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ObjDL struct
 */

/*
 * objdl.hxx
 *
 *  Created on: 11.01.2011
 *      Author: jason
 */

#ifndef OBJDL_HXX_
#define OBJDL_HXX_

#include "game_object.hxx"

/**
 * The ObjDL is an indirect reference that is notified on a
 * GameObject's deletion, nullifing the internal pointer.
 *
 * ObjDL (Object Death Listener) is a simple struct intended to
 * replace ObjRef for several reasons:
 * + ObjRef's detection algorithm is inefficient (O(n)) and detects
 *   the change after the fact.
 * + The change won't even be detected if a new object happens to
 *   get the same address before a scan
 * + The template causes issues.
 * + The operator overloading causes issues.
 * + It also encourages inefficient coding (calling operator-> many,
 *   many times).
 *
 * ObjDL works by attaching itself to the reference object, creating
 * a linked list of listeners. Upon death, the object sets the internal
 * reference to NULL.
 *
 * This class does NOT extend AObject, to avoid the overhead. Tcl can
 * check for object death by polling the isDead property every frame,
 * since the GameField introduces a two-frame delay between object
 * predeletion and actual deletion.
 */
struct ObjDL {
  GameObject* ref; ///< The GameObject linked to, or NULL
  ObjDL* nxt, ///< The next ObjDL in the linked list
       * prv; ///< The previous ObjDL in the linked list

  /** Creates an ObjDL initialized to NULL. */
  ObjDL()
  : ref(NULL)
  { }

  /** Creatse an ObjDL initialized to an arbitrary object,
   * which can be NULL.
   */
  explicit ObjDL(GameObject* go)
  : ref(go), nxt(go? go->listeners : NULL), prv(NULL)
  {
    if (go) go->listeners=this;
    if (nxt) nxt->prv=this;
  }

  ~ObjDL() {
    assign(NULL);
  }

  /** Performs a logical copy of the given ObjDL.
   * The two will not be a binary copy of each other;
   * only the reference will match.
   */
  ObjDL(const ObjDL& other)
  : ref(NULL)
  {
    assign(other.ref);
  }

  /** Performs comparison on internal reference */
  bool operator<(const ObjDL& that) const {
    return ref < that.ref;
  }

  /** Performs comparison on internal reference */
  bool operator==(const ObjDL& that) const {
    return ref==that.ref;
  }

  /** Change the reference.
   * Automatically delinks from the old reference, if there is any,
   * and links to the new one, if non-NULL.
   */
  void assign(GameObject* go) {
    if (ref) {
      //Cancel old reference
      if (prv) prv->nxt=nxt;
      else     ref->listeners=nxt;
      if (nxt) nxt->prv=prv;
    }
    //Assign
    ref=go;
    //Push to head of new object
    if (ref) {
      prv=NULL;
      nxt=go->listeners;
      if (nxt) nxt->prv=this;
      go->listeners=this;
    }
  }

  /** Performs a logical assignment.
   * After the call, the two will not be a binary copy of each other;
   * only the reference will match.
   */
  ObjDL& operator=(const ObjDL& that) {
    assign(that.ref);
    return *this;
  }
};

#endif /* OBJDL_HXX_ */
