/**
 * @file
 * @author Jason Lingle
 * @brief Makes C automatic/static arrays accessible to trusted Tcl.
 */

/*
 * array.hxx
 * Make C arrays accessible to trusted Tcl.
 *
 *  Created on: 12.07.2010
 *      Author: jason
 */

#ifndef ARRAY_HXX_
#define ARRAY_HXX_

#include <stdexcept>
#include <exception>

#include "src/core/aobject.hxx"

/** Represents an automatically-allocated array of fixed
 * size. Tcl can also create them, in which case the data
 * is automatically deleted.
 * Uses of this in the definition must always be C++-outgoing,
 * immediate, and steal.
 */
template<typename T, unsigned S>
class StaticArray: public AObject {
  T* data;
  bool del;

  public:
  /** Tcl constructor */
  StaticArray() : data(new T[S]), del(true) {}
  /** Construct the array on the given array */
  StaticArray(T* d) : data(d), del(false) {}

  virtual ~StaticArray() { if (del) delete[] data; }

  operator T*() { return data; }

  /** Array read */
  T at(unsigned i) {
    if (i>=0 && i<S) return data[i];
    throw std::range_error("Out of bounds");
  }

  /** Array set */
  T set(unsigned i, T t) {
    if (i>=0 && i<S) return data[i]=t;
    throw std::range_error("Out of bounds");
  }

  /** Array size */
  unsigned size() { return S; }

  /** Equality is determined solely by comparing the pointers. */
  bool eq(StaticArray<T,S>* other) { return data==other->data; }
};

/** Represents a dynamically-allocated array.
 *
 * The glue
 * definition should ensure that Tcl always has control
 * over this class. Tcl is responsible for knowing whether
 * to delete the contents.
 *
 * The array does not keep track of allocation size.
 * If Tcl "allocates" an array of size 0, a NULL array
 * is actually created.
 */
template<typename T>
class DynArray: public AObject {
  T* data;

  public:
  /** Constructs a DynArray on the given array */
  DynArray(T* d) : data(d) {}
  /** Allocates an array of the given size */
  explicit DynArray(unsigned sz) : data(sz? new T[sz] : 0) {}
  operator T*() { return data; }

  T at(unsigned i) { return data[i]; } ///< Array read
  T set(unsigned i, T t) { return data[i]=t; } ///< Array write
  bool eq(DynArray<T>* other) { return other->data==data; } ///< Pointer equality

  void del() { delete[] data; } ///< Deallocation
};

#endif /* ARRAY_HXX_ */
