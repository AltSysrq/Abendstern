#ifndef EXPLODE_LISTENER_HXX_
#define EXPLODE_LISTENER_HXX_

/**
 * @file
 * @author Jason Lingle
 * @date 2012.03.08
 * @brief Contains the ExplodeListener class.
 */

/**
 * Allows subclasses (usually parts of the networking system) to find out that
 * a certain object has exploded.
 *
 * This works with any type that has an explodeListeners field of type
 *   ExplodeListener<T>*
 */
template<typename T>
class ExplodeListener {
public:
  //Linked list of listeners; prv is a pointer to a nxt pointer
  ExplodeListener<T>** prv, * nxt;

  ExplodeListener<T>(const ExplodeListener<T>&);

  /**
   * Constructs the ExplodeListener on the given object, which may be NULL.
   */
  ExplodeListener<T>(T* attachTo = NULL)
  : prv(NULL), nxt(NULL)
  {
    if (attachTo) {
      prv = &attachTo->explodeListeners;
      nxt = *prv;
      attachTo->explodeListeners = this;
    }
  }

  virtual ~ExplodeListener<T>() {
    if (prv)
      *prv = nxt;
    if (nxt)
      nxt->prv = prv;
  }

  /**
   * Called when the listened object explodes.
   */
  virtual void exploded(T*) throw() = 0;
};

#endif /* EXPLODE_LISTENER_HXX_ */
