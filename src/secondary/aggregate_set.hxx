/**
 * @file
 * @author Jason Lingle
 * @brief Contains the AggregateSet template class
 */

#ifndef AGGREGATE_SET_HXX_
#define AGGREGATE_SET_HXX_

#include <set>
#include <map>
#include <utility>
#include <iterator>

#include "src/core/aobject.hxx"

/** An AggregateSet combines multiple source sets into a
 * single read-only collection. Sources are called "clients",
 * and actively provide entries for the set. A client can
 * perform the following operations:
 * \verbatim
 *   reset -- all references to its items are cleared
 *   add -- adds a new item to the set
 *   delete -- the client ceases to exist
 * \endverbatim
 */
template <typename T, typename C>
class AggregateSet: public AObject {
  private:
  typedef std::pair<T, std::set<C> > item;

  //All items present
  typedef std::set<item*> contents_t;
  contents_t contents;
  //Map data to item
  typedef std::map<T, item*> d2ct;
  d2ct data2Contents;
  //Map client to items
  typedef std::map<C, std::set<item*> > c2ct;
  c2ct client2Contents;

  //A set of all existent Aggregate sets of the same type
  static std::set<AggregateSet<T,C>*> allSets;

  public:
  /**
   * Inserts a new datum from the given client.
   * @param dat Datum to insert
   * @param cln Client providing the datum
   */
  void insert(const T& dat, const C& cln) {
    typename d2ct::iterator it=data2Contents.find(dat);
    item* i;
    if (it == data2Contents.end()) {
      std::set<C> s;
      i=new item(dat, s);
      data2Contents.insert(make_pair(dat, i));
      contents.insert(i);
    } else i = (*it).second;
    i->second.insert(cln);
    client2Contents[cln].insert(i);
  }

  /**
   * Clears all association of the given client with its data.
   * Data that have no association after this call are removed.
   * @param cln Client being cleared/reset
   */
  void clear(const C& cln) {
    typename c2ct::iterator it=client2Contents.find(cln);
    if (it != client2Contents.end()) {
      for (typename std::set<item*>::iterator i = (*it).second.begin();
      i != (*it).second.end(); ++i) {
        //Remove the client from the owners
        item* im = *i;
        typename std::set<C>::iterator imit(im->second.find(cln));
        if (imit != im->second.end())
          im->second.erase(imit);
        if (!im->second.size()) {
          //The item is no longer referenced
          data2Contents.erase(data2Contents.find(im->first));
          contents.erase(contents.find(im));
          delete im;
        }
      }
      (*it).second.clear();
    }
  }

  /**
   * Removes all traces of the given client.
   * Implicitly calls clear().
   * @param cln Client to delete
   * @see clear(cosnt C&)
   */
  void delClient(const C& cln) {
    clear(cln);
    typename c2ct::iterator c2ci = client2Contents.find(cln);
    if (c2ci != client2Contents.end())
      client2Contents.erase(c2ci);
  }

  /**
   * Removes all occurrances of the specified datum.
   * @param dat The datum to purge
   */
  void purge(const T& dat) {
    typename d2ct::iterator it(data2Contents.find(dat));
    if (it != data2Contents.end()) {
      item* i((*it).second);
      //Remove from client2Contents lists
      for (typename c2ct::iterator cit=client2Contents.begin();
           cit != client2Contents.end(); ++cit)
      {
        typename std::set<item*>::iterator ciit((*cit).second.find(i));
        if (ciit != (*cit).second.end())
          (*cit).second.erase(ciit);
      }
      contents.erase(contents.find((*it).second));
      data2Contents.erase(it);
      delete i;
    }
  }

  /**
   * Purges the given datum from ALL AggregateSet<T,C>s in existence.
   * @param dat Datum to exhaustively purge.
   */
  void superPurge(const T& dat) {
    for (typename std::set<AggregateSet<T,C>*>::iterator it=allSets.begin();
         it != allSets.end(); ++it)
    {
      (*it)->purge(dat);
    }
  }

  /**
   * Constructs a new, empty AggregateSet and inserts itself
   * into the list of all AggregateSets.
   */
  AggregateSet() {
    allSets.insert(this);
  }

  virtual ~AggregateSet() {
    for (typename contents_t::iterator it=contents.begin();
         it != contents.end(); ++it)
      delete *it;
    allSets.erase(allSets.find(this));
  }

  /** Bidirectional RW iterator */
  class iterator {
    friend class AggregateSet<T,C>;
    private:
    typename contents_t::iterator it;
    iterator(const typename contents_t::iterator& i)
    : it(i)
    { }

    public:
    typedef T value_type;
    typedef T& reference;
    typedef T* pointer;
    typedef std::bidirectional_iterator_tag iterator_category;
    //Why must this be defined for non-random-access iterators?
    typedef unsigned difference_type;

    /** Copy constructor */
    iterator(const iterator& i)
    : it(i.it)
    { }

    /** Null constructor.
     * An iterator constructed with this is not valid.
     */
    iterator () : it() {}

    iterator& operator++() {
      ++it;
      return *this;
    }

    iterator operator++(int) {
      iterator tmp(*this);
      ++it;
      return tmp;
    }

    iterator& operator--() {
      --it;
      return *this;
    }

    iterator operator--(int) {
      iterator tmp(*this);
      --it;
      return tmp;
    }

    T& operator*() {
      return (*it)->first;
    }

    T* operator->() {
      return &operator*();
    }

    bool operator==(const iterator& i) const {
      return i.it==this->it;
    }

    bool operator!=(const iterator& i) const {
      return this->it != i.it;
    }

    iterator& operator=(const iterator& i) {
      it=i.it;
      return *this;
    }
  };

  /** Iterator to beginning */
  iterator begin() {
    return iterator(contents.begin());
  }

  /** Iterator to end */
  iterator end() {
    return iterator(contents.end());
  }

  /** Reverse iterator to end */
  iterator rbegin() {
    return iterator(contents.rbegin());
  }

  /** Reverse iterator to beginning */
  iterator rend() {
    return iterator(contents.rend());
  }

  /** Return an iterator pointing to the given datum,
   * or end() otherwise.
   */
  iterator find(const T& t) {
    typename d2ct::iterator d2ci = data2Contents.find(t);
    if (d2ci == data2Contents.end()) return end();
    return iterator(contents.find((*d2ci).second));
  }

  /** Return number of data contained. */
  size_t size() const {
    return contents.size();
  }
};

/** Force automatic instantiation of the static allSets member. */
template<typename T, typename C>
std::set<AggregateSet<T,C>*> AggregateSet<T,C>::allSets;

#endif

