/**
 * @file
 * @author Jason Lingle
 * @brief Contains the ConfReg configuration centralisation system
 */

#ifndef CONFREG_HXX_
#define CONFREG_HXX_

#include <libconfig.h++>
#include <map>
#include <list>
#include <string>
#include <stdexcept>
#include <exception>

#include "src/core/aobject.hxx"

/** Contains details on a root of the ConfReg. */
struct ConfigRegEntry {
  std::string filename;
  bool modified;
  libconfig::Config* conf;
};

/**
 * Like libconfig::SettingNotFoundException.
 *
 * The libconfig++ SettingNotFoundException is completely useless,
 * as its what() function only returns "SettingNotFoundException",
 * telling us nothing.
 * Unfortunately, it is only practical to do the switch on the Tcl
 * accessors...
 */
class NoSuchSettingException: public std::runtime_error {
  public:
  explicit NoSuchSettingException(const char* msg="") : std::runtime_error(msg) {}
};

/** The ConfReg (Configuration Registery) provides a simple way to
 * organize Abendstern's configuration files. It is basically a map
 * from strings to libconfig::Setting roots.
 * The main reason it exists is so that all instances of libconfig::Config
 * are common across the entire program, and so that Tcl has a good way to
 * access the configuration.
 */
class ConfReg: public AObject, private std::map<std::string, ConfigRegEntry> {
  std::list<std::string> whitelist;
  bool requireWhitelist;

  public:
  /** Creates a new, empty ConfReg. */
  ConfReg()
  : AObject(), std::map<std::string, ConfigRegEntry>(),
    requireWhitelist(false)
  {}

  virtual ~ConfReg();

  /** Opens the given filename to the given key. If something is at
   * that key already, it is closed, then replaced.
   * Any exceptions from libconfig propagate up.
   */
  void open(const std::string& filename, const std::string& key);
  /**
   * Registers the given filename to the given key. If something is at that key
   * already, it is closed, then replaced.
   *
   * The file in question is not actually loaded until the key is
   * accessed. When such a load is triggered, any errors which occur are logged
   * to stderr, but otherwise ignored, resulting in an apparently empty config.
   */
  void openLazily(const std::string& filename, const std::string& key);
  /** Creates a new libconfig++ mount. The file is not read in. */
  void create(const std::string& filename, const std::string& key);
  /** Closes the given key, if it exists. If modified, the
   * file is written out first, unless its filename is empty.
   */
  void close(const std::string&);
  /** Closes all configs. */
  void closeAll();

  /** Accesses the given setting root. Throws libconfig::SettingNotFoundException
   * if there is no such root.
   */
  libconfig::Setting& operator[](const std::string&);

  /** Sets the given root as "modified", so that it is written
   * when closed or resynched.
   */
  void modify(const std::string&);
  /** Sets modified to false for the given root. */
  void unmodify(const std::string&);
  /** Saves the given conf without closing it.
   * Does nothing if not modified, or if the filename is empty.
   */
  void sync(const std::string&);
  /** Calls sync() on all roots */
  void syncAll();

  /**
   * Reload the config from disk (even if "not modified").
   * Does nothing if the filename is empty, or if the key has not yet been
   * loaded.
   */
  void revert(const std::string&);
  /** Call revert() on all roots */
  void revertAll();

  /**
   * Changes the name of the backing file for the given
   * root, and sets its "modified" status.
   */
  void renameFile(const std::string& root, const std::string& filename);

  /**
   * Returns whether the given key, which must exist, has been loaded.
   *
   * @throws NoSuchSettingException if the key does not exist.
   */
  bool loaded(const std::string& key);

  /* The below functions are mainly intended for Tcl, but are usable by
   * other code.
   */
  /** Returns true if a setting exists with the given path. */
  bool exists(const std::string& path);
  /** Returns the boolean value at the given path. */
  bool getBool(const std::string& path);
  /** Returns the int value at the given path. */
  int getInt(const std::string&);
  /** Returns the float value at the given path. */
  float getFloat(const std::string&);
  /** Returns the string value at the given path. */
  std::string getStr(const std::string&);
  /** Sets the boolean value at the given path. */
  void setBool(const std::string&, bool);
  /** Sets the int value at the given path. */
  void setInt(const std::string&, int);
  /** Sets the float value at the given path. */
  void setFloat(const std::string&, float);
  /** Sets the string value at the given path. */
  void setString(const std::string& path, const std::string& val);
  /** Recursively copies the source setting into the destination.
   * This is a front-end to confcpy(), and makes the same assumptions.
   * @param dst The destination for the copy
   * @param src The source of the copy
   * @see confcpy()
   */
  void copy(const std::string& dst, const std::string& src);

  /** Returns the name of the setting at the given path. */
  std::string getName(const std::string&);
  /** Adds a new setting of the given type.
   * The initial value will be the libconfig default.
   * @param parent Group Setting to insert into
   * @param child Name of new Setting
   * @param type Type of new Setting
   */
  void add(const std::string& parent, const std::string& child, libconfig::Setting::Type type);
  /** Adds a boolean Setting to the given group.
   *
   * @param parent Group Setting to insert into
   * @param child Name of new Setting
   * @param v Initial value for new Setting
   */
  void addBool(const std::string& parent, const std::string& child, bool v);
  /** Adds an int Setting to the given group.
   *
   * @param parent Group Setting to insert into
   * @param child Name of new Setting
   * @param v Initial value for new Setting
   */
  void addInt(const std::string& parent, const std::string& child, int v);
  /** Adds a float Setting to the given group.
   *
   * @param parent Group Setting to insert into
   * @param child Name of new Setting
   * @param v Initial value for new Setting
   */
  void addFloat(const std::string& parent, const std::string& child, float v);
  /** Adds a string Setting to the given group.
   *
   * @param parent Group Setting to insert into
   * @param child Name of new Setting
   * @param v Initial value for new Setting
   */
  void addString(const std::string& parent, const std::string& child, const std::string& v);
  /** Removes the given setting (and any children it has). */
  void remove(const std::string&);
  /** Adds a new Setting of the given type to an indexed aggregate.
   * The initial value will be the libconfig default
   *
   * @param parent A List or Array to append to
   * @param type The type of the new Setting
   */
  void pushBack(const std::string& parent, libconfig::Setting::Type type);
  /** Adds a new boolean Setting to the given indexed aggregate.
   *
   * @param parent A List or Array to append to
   * @param v Initial value for new Setting
   */
  void pushBackBool(const std::string& parent, bool v);
  /** Adds a new int Setting to the given indexed aggregate.
   *
   * @param parent A List or Array to append to
   * @param v Initial value for new Setting
   */
  void pushBackInt(const std::string&, int v);
  /** Adds a new float Setting to the given indexed aggregate.
   *
   * @param parent A List or Array to append to
   * @param v Initial value for new Setting
   */
  void pushBackFloat(const std::string&, float v);
  /** Adds a new string Setting to the given indexed aggregate.
   *
   * @param parent A List or Array to append to
   * @param v Initial value for new Setting
   */
  void pushBackString(const std::string&, const std::string& v);
  /** Removes the sub-Setting at the given index from the parent.
   *
   * @param parent Aggregate to remove from
   * @param ix Index to remove
   */
  void remix(const std::string& parent, unsigned ix);
  /** Returns the type of the Setting at the given path. */
  libconfig::Setting::Type getType(const std::string&);
  /** Returns the length of the Aggregate at the given path. */
  int getLength(const std::string&);
  /** Returns whether the Setting at the given path is a group. */
  bool isGroup(const std::string&);
  /** Returns whether the Setting at the given path is an array. */
  bool isArray(const std::string&);
  /** Returns whether the Setting at the given path is a list. */
  bool isList(const std::string&);
  /** Returns whether the Setting at the given path is a list, array, or group. */
  bool isAggregate(const std::string&);
  /** Returns whether the Setting at the given path is a bool, int, float, or string */
  bool isScalar(const std::string&);
  /** Returns whether the Setting at the given path is an int or float */
  bool isNumber(const std::string&);
  /** Returns the line number of the source on which the Setting with the
   * given path is defined.
   */
  int getSourceLine(const std::string&);

  /** Returns the setting at the specified path.
   * This DOES honour the whitelist restrictions.
   *
   * @param path Path to the Setting, subject to whitelist restrictions.
   * @param modify If true, flag the root of path as modified
   */
  libconfig::Setting& lookup(const std::string&, bool modify=false);

  /** Adds the given prefix to the whitelist.
   * Privilaged code only.
   */
  void addToWhitelist(const std::string&);
  /** Removes the given prefix from the whitelist.
   * Privilaged code only.
   */
  void removeFromWhitelist(const std::string&);
  /** Clears the whitelist.
   * Privilaged code only.
   */
  void clearWhitelist();

  /** If set to true, all accesses will be blocked if
   * they do not start with a prefix in the whitelist.
   * This blocking manifests itself in a simulation
   * of a missing setting.
   * Privilaged code only.
   */
  void setWhitelistOnly(bool);

  private:
  void whitelistCheck(const std::string&);
  void load(ConfigRegEntry&);
};

/** Copies the value of the source setting into the destination setting.
 * It is assumed that the destination is already the right type and
 * is empty if non-scalar.
 *
 * @param dst Destination of copy
 * @param src Source of copy
 */
void confcpy(libconfig::Setting& dst, const libconfig::Setting& src);

/** Copies the value of the source setting into the destination setting.
 * It is assumed that the destination is already the right type and
 * is empty if non-scalar.
 *
 * @param dst Destination of copy
 * @param src Source of copy
 */
void confcpy(const std::string& dst, const std::string& src);

#endif /*CONFREG_HXX_*/
