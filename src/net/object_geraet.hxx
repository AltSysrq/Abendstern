/**
 * @file
 * @author Jason Lingle
 * @date 2012.02.25
 * @brief Contains classes for communicating GameObjects over the network.
 */
#ifndef OBJECT_GERAET_HXX_
#define OBJECT_GERAET_HXX_

#include "network_geraet.hxx"
#include "block_geraet.hxx"

#include "src/sim/objdl.hxx"

class GameObject;

/**
 * Manages a remotely-hosted GameObject.
 *
 * Subclasses must provide functions for constructing new copies
 * of the object, for decoding and updating it, and for destroying
 * it appropriately.
 */
class ImportedGameObject: public InputBlockGeraet {
protected:
  /**
   * The GameField* the object is to be placed into.
   */
  GameField*const field;

  /**
   * The local copy of the GameObject being imported.
   *
   * If this is NULL, either the Gerät data has not yet been
   * initialised, or the subclass deleted it.
   *
   * If this is non-NULL when the destructor runs, the object
   * is collided with itself, removed from the field, and
   * queued for deletion.
   *
   * @see created
   * @see construct()
   * @see update()
   */
  GameObject* object;
  /**
   * Whether object has been created.
   *
   * If true, and object is NULL, no new updates will be processed.
   * This is set to false initially, and to true after the call to
   * construct().
   */
  bool created;

  /**
   * Creates an ImportedGameObject of the given size on the given connection.
   *
   * object is initialised to NULL and created to false.
   */
  ImportedGameObject(unsigned, NetworkConnection*);

public:
  virtual ~ImportedGameObject();

protected:
  virtual void modified() throw();

  /**
   * Called the first time that the Gerät data is modified.
   * This function must set object to an instance of the appropriate type
   * of GameObject, but not add it to the field.
   *
   * When this is called, created will be false, but it will be set to
   * true after the call.
   *
   * After this call, the dirty bitset is cleared.
   */
  virtual void construct() throw() = 0;
  /**
   * Decodes modified data and updates the object based upon that data.
   *
   * This function may destroy the object by calling destroy().
   */
  virtual void update() throw() = 0;

  /**
   * Properly destroys the object, including queueing it for deletion and
   * removing it from the field.
   * Behaviour is undefined if object==NULL.
   */
  void destroy() throw();
};

/**
 * Manages the exportation of a local GameObject.
 *
 * Efficient transmission of updates is accomplished by maintaining a copy
 * of the object within the NetworkConnection-specific GameField, allowing
 * update decisions to be made based on a comparison between the two.
 */
class ExportedGameObject: public OutputBlockGeraet {
private:
  //Minimum time before pushing another update
  signed timeUntilNextUpdate;
  //The status we believe the local object to be in.
  bool alive;

protected:
  /**
   * The local object within the primary GameField.
   * When this becomes NULL, destroyRemote() is called, and the device will
   * remove itself once changes are resynchronised with the remote peer.
   */
  ObjDL local;
  /**
   * The local "remote mirror" object. It is kept in the NetworkConnection-
   * specific GameField, and is only updated to match the local object when
   * changes visible to the remote peer are made.
   *
   * This is automatically deallocated in the destructor.
   */
  GameObject*const remote;

  /**
   * Constructs an ExportedGameObject.
   * @param sz the size of the block Gerät
   * @param cxn the NetworkConnection to use
   * @param local the real local object to track
   * @param remote a "remote" copy of local
   */
  ExportedGameObject(unsigned sz, NetworkConnection* cxn,
                     GameObject* local, GameObject* remote);

public:
  virtual ~ExportedGameObject();

  virtual void update(unsigned) throw();

protected:
  /**
   * Compares the real local object with the remote mirror, and returns whether
   * it is necessary to send an update packet.
   *
   * If it returns true, updateRemote() will be called.
   */
  virtual bool shouldUpdate() const throw() = 0;
  /**
   * Called to push updates about the object to the remote peer.
   * This must encode the object data into the state array, and update the
   * remote mirror to match the actual local object.
   *
   * The dirty flag is automatically set after this call.
   */
  virtual void updateRemote() throw() = 0;
  /**
   * Called when the local object ceases to exist.
   * This must encode deletion information into the state.
   *
   * The dirty flag is automatically set after this call.
   *
   * After this call, the Gerät waits until the deletion is confirmed
   * received, then closes itself.
   */
  virtual void destroyRemote() throw() { };
};

#endif /* OBJECT_GERAET_HXX_ */
