/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ObjectIntref class
 */

/*
 * object_intref.hxx
 *
 *  Created on: 14.02.2011
 *      Author: jason
 */

#ifndef OBJECT_INTREF_HXX_
#define OBJECT_INTREF_HXX_

class GameObject;

/** The ObjectIntref allows storing references to objects in
 * temporary storage systems that do not support pointers and
 * where converting the pointer to an integer is considered a
 * bad idea.
 *
 * An example is the inter-AI communication system, which uses
 * libconfig as a back-end. An ObjectIntref allows converting
 * the pointer to an integer which may be safely passed around,
 * and is independant of the actual pointer.
 *
 * Third parties who have the interger may look the current value
 * of the reference up via static functions.
 *
 * The class stores a reference to the target pointer internally,
 * so it must be managed the same way as the pointer (eg, in the
 * same automatic memory).
 */
class ObjectIntref {
  GameObject*const& pointer;
  const int reference;

  public:
  /** Creates an ObjectIntref to the given GameObject. */
  ObjectIntref(GameObject*const&);
  ~ObjectIntref();

  /** Returns the integer used to refer
   * to the pointer.
   */
  int getReference() const { return reference; }

  /** Returns the GameObject* corresponding to the specified
   * integer, or NULL if there is no longer anything there.
   */
  static GameObject* get(int);
};

#endif /* OBJECT_INTREF_HXX_ */
