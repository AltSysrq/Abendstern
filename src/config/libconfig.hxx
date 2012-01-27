/**
 * @file
 * @author Jason Lingle
 *
 * @brief Header for the reimplementation of libconfig++.
 *
 * libconfig++, the original configuration library used by Abendstern,
 * has a number of problems which motivated this reimplementation:
 * 1. libconfig uses a linear search to find named members,
 *    which is a serious bottleneck in ship loading
 * 2. libconfig++'s exception messages are useless (as per the C++ standard
 *    library); we really want an actual description
 * 3. libconfig++'s Setting has an implicit operator const string& which causes
 *    serious problems in MSVC++
 * 4. libconfig is intended to be a separate library, which is a bit needless
 *    given its very small size
 * 5. Though not a problem of libconfig itself, Abendstern's reliance on using
 *    it for ships and keeping Configs of them loaded at all times will eventually
 *    become a memory issue without some kind of secondary storage
 *
 * This reimplementation is almost a strict superset of libconfig (both at file and
 * source level), with a few exceptions. Incompatibilities are:
 * 1. The Setting class does not have an operator const string&
 * 2. The Config class cannot operate on FILE*s
 * 3. There is no support for the include directive (or any of the related functions)
 * 4. Autoconversion cannot be performed
 * 5. Custom output formatting (including indentation) is not supported
 * 6. The Setting::TypeUnknown value for Setting::Type is not present in this implementation.
 * 7. Floating-points are stored internally as floats instead of doubles
 * 8. Source file and line information is not stored in Settings
 * 9. libconfig::garbageCollection() must be run regularly to deallocate Settings
 *
 * The file format is mostly the same, except for some relaxations:
 * + The characters =:;, are all considered whitespace, except that , is used to separate
 *   strings.
 * + Only the first character of boolean values matters (tyj = true, fn = false)
 * + Identifiers may begin with numerals and hyphen
 * + The alert (backslash-A) escape code is supported
 *
 * The library provides a number of additional extensions:
 * + Offloading of data onto a temporary disk file
 * + Setting the amount of primary storage to use
 * + Querying amount of storage used
 * + The exceptions can be constructed by external code
 *
 * Additionally, files written by this implementation are more compact than
 * libconfig++'s, at the cost of being less human-readable.
 *
 * Note that there is no abconfig==>libconfig compatibility; in particular, files written
 * by this implementation take full advantage of the format relaxations; for example, the
 * config represented by the libconfig file
 * \verbatim
 *   bool = true;
 *   group: { list = ("foo", [1,2,3]); array = [4.0,5.0,6.0]; };
 * \endverbatim
 * would be rewritten by abconfig as something like
 * \verbatim
 *   bool       t
 *   group      {
 *    list      ("foo", [1 2 3])
 *    array     [4.0 5.0 6.0]
 *   }
 * \endverbatim
 */

/*
 * libconfig.hxx
 *
 *  Created on: 30.07.2011
 *      Author: jason
 */

#ifndef LIBCONFIG_HXX_
#define LIBCONFIG_HXX_

#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>

/** Contains the libconfig++ reimplementation */
namespace libconfig {
  /** Don't use this!
   *
   * @see size_t
   */
  template<bool longIsPtr> struct size_t_tmpl {};
  /** Don't use this directly!
   *
   * @see size_t
   */
  template<> struct size_t_tmpl<false> {
    typedef unsigned long long t;
  };
  /** Don't use this directly!
   *
   * @see size_t
   */
  template<> struct size_t_tmpl<true> {
    typedef unsigned long t;
  };
  /**
   * Represents an unsigned integer with the same size as a pointer.
   *
   * Normally, this would simply be std::size_t. However, as far as I have
   * heard, Win64 has size_t as 32-bit, AND long is 32-bit on that platform
   * as well.
   *
   * This works by passing a comparison between pointer size and long to
   * the size_t_tmpl template, which has specific "implementations" for
   * both cases which typedef the appropriate response (ie, if they are
   * not the same size, explicitly use long long, otherwise use long).
   */
  typedef size_t_tmpl<sizeof(void*)==sizeof(unsigned long)>::t iptr;

  /** The base class of all libconfig++ exceptions. */
  class ConfigException: public std::runtime_error {
    ConfigException& operator=(const ConfigException&); ///<Not implemented
    protected:
    /** Constructs a ConfigException with the given description. */
    explicit ConfigException(const std::string& what) : std::runtime_error(what) {}
    ConfigException(const ConfigException& that) : std::runtime_error(that) {}

    public:
    //Need to explicitly override due to the throw()
    virtual ~ConfigException() throw() {}
  };

  /** The base class of setting-originating exceptions. */
  class SettingException: public ConfigException {
    SettingException& operator=(const SettingException&); ///<Not implemented

    const std::string path;

    protected:
    SettingException(const SettingException& that)
    : ConfigException(that), path(that.path) {}
    /** Constructs a SettingException with the given description and path. */
    explicit SettingException(const std::string& what, const std::string& where)
    : ConfigException(where + ": " + what), path(where) {}

    public:
    /** Returns the path to the setting that triggered the exception. */
    const char* getPath() const { return path.c_str(); }
    //Need to explicitly override due to the throw()
    virtual ~SettingException() throw() {}
  };

  /** Thrown when an attempt to access a Setting with an incopatible type is made. */
  class SettingTypeException: public SettingException {
    SettingTypeException& operator=(const SettingTypeException&); ///<Not implemented

    public:
    SettingTypeException(const SettingTypeException& that) : SettingException(that) {}
    /** Constructs a SettingTypeException with the given description and path. */
    SettingTypeException(const std::string& what, const std::string& where)
    : SettingException(what,where) {}
    //Need to explicitly override due to the throw()
    virtual ~SettingTypeException() throw() {}
  };

  /** Thrown when an attempt is made to access a non-existent subSetting. */
  class SettingNotFoundException: public SettingException {
    SettingNotFoundException& operator=(const SettingNotFoundException&); ///<Not implemented

    public:
    SettingNotFoundException(const SettingNotFoundException& that) : SettingException(that) {}
    /** Constructs a SettingNotFoundException with the given description and path. */
    SettingNotFoundException(const std::string& what, const std::string& where)
    : SettingException(what,where) {}
    //Need to explicitly override due to the throw()
    virtual ~SettingNotFoundException() throw() {}
  };

  /** Thrown when an attempt is made to use a duplicate or invalid name for a Setting. */
  class SettingNameException: public SettingException {
    SettingNameException& operator=(const SettingNameException&); ///<Not implemented

    public:
    SettingNameException(const SettingNameException& that) : SettingException(that) {}
    /** Constructs a SettingNameException with the given description and path. */
    SettingNameException(const std::string& what, const std::string& where)
    : SettingException(what,where) {}
    //Need to explicitly override due to the throw()
    virtual ~SettingNameException() throw() {}
  };

  /** Thrown when loading a file with invalid syntax. */
  class ParseException: public ConfigException {
    ParseException& operator=(const ParseException&); ///<Not implemented

    std::string error;
    std::string file;
    unsigned line;

    public:
    ParseException(const ParseException& that)
    : ConfigException(that),
      error(that.error), file(that.file), line(that.line)
    { }
    /** Constructs a ParseException with the given description, file, and line number */
    explicit ParseException(const std::string& what, const std::string& f, unsigned l)
    : ConfigException(getDetails(what,f,l)), error(what), file(f), line(l) {}

    /** Returns the base error message */
    const char* getError() const { return error.c_str(); }
    /** Returns the name of the file operated on */
    const char* getFile() const { return file.c_str(); }
    /** Returns the line number the error was encountered on */
    unsigned getLine() const { return line; }

    //Need to explicitly override due to the throw()
    virtual ~ParseException() throw() {}

    private:
    /** Returns the detailed error message, of the format
     * file:line: message
     */
    std::string getDetails(const std::string& what, const std::string& file, unsigned line) const {
      std::ostringstream os;
      os << file << ':' << line << ": " << what;
      return os.str();
    }
  };

  /** Thrown when an IO error occurs while reading or writing a file. */
  class FileIOException: public ConfigException {
    FileIOException& operator=(const FileIOException&); ///<Not implemented

    public:
    FileIOException(const FileIOException& that) : ConfigException(that) {}
    /** Constructs a FileIOException with the given description. */
    explicit FileIOException(const std::string& what)
    : ConfigException(what) {}
    //Need to explicitly override due to the throw()
    virtual ~FileIOException() throw() {}
  };

  class Setting;

  /** Contains the function entry-points for manipulating a config file. */
  class Config {
    Setting* root;

    public:
    /** Constructs a new, empty config file. */
    Config();
    ~Config();

    /** Reads data from the given file.
     * @param filename The name of the file from which to read
     * @throw FileIOException if the file cannot be opened or an error occurs while reading
     * @throw ParseException if syntax errors are encountered
     * @throw SettingNameException if duplicate names are encountered
     * @throw SettingTypeException if type mismatches are encountered
     */
    void readFile(const char* filename);
    /** Writes data to the given file.
     * @param filename The name of the file to which to write
     * @throw FileIOException if the file cannot be opened or an error occurs while writing
     */
    void writeFile(const char* filename) const;

    /** Reads data from the given string.
     * @param str Input data
     * @param name Name of input
     * @throw ParseException if syntax errors are encountered
     * @throw SettingNameException if duplicate names are encountered
     * @throw SettingTypeException if type mismatches are encountered
     */
    void readString(const char* str, const char* name = "unknown");
    /** @see readString(const char*) */
    void readString(const std::string& str, const char* name = "unknown") { readString(str.c_str(), name); }

    /** Writes data to a string and returns it. */
    std::string writeString() const;

    /** Returns the root Setting for this config.
     */
    Setting& getRoot();
    /** Returns the root Setting for this config. */
    const Setting& getRoot() const;

    /** Returns the Setting with the given path.
     * @throw SettingNotFoundException if it does not exist
     */
    Setting& lookup(const char*);
    /** @see lookup(const char*) */
    Setting& lookup(const std::string&);
    /** @see lookup(const char*) */
    const Setting& lookup(const char*) const;
    /** @see lookup(const char*) */
    const Setting& lookup(const std::string&) const;

    /** Returns whether a Setting exists at the given path. */
    bool exists(const char*) const;
    /** @see exists(const char*) */
    bool exists(const std::string&) const;

    /** Looks up the given Setting and stores its value into the
     * given reference if it exists.
     * @param path Path to the desired setting
     * @param var Target for Setting's value (unchanged if not found)
     * @return True if the Setting was found and was of the correct type
     */
    bool lookupValue(const char* path, bool& var) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const std::string&, bool&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const char*, signed&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const std::string&, signed&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const char*, unsigned&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const std::string&, unsigned&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const char*, signed long long&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const std::string&, signed long long&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const char*, unsigned long long&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const std::string&, unsigned long long&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const char*, float&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const std::string&, float&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const char*, double&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const std::string&, double&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const char*, const char*&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const std::string&, const char*&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const char*, std::string&) const;
    /** @see lookupValue(const char*,bool&) const */
    bool lookupValue(const std::string&, std::string&) const;
  };

  /** A pseudo-class that allows access to setting objects.
   *
   * While this class is object-like, pointers and references provided
   * by this implementation will never point to actual instances
   * of this class (in fact, it has no public constructors). It is
   * not even guaranteed that the memory pointed to by a Setting& is
   * in mapped memory. The implementation relies on the static binding
   * of the functions, which do not require the object to be dereferenced.
   *
   * However, a NULL pointer will never be returned as a valid Setting&,
   * so that assumptions thereabout will still be valid.
   *
   * All storage for returned const char*'s is managed internally. It is
   * not to be freed by calling code, and will only be guaranteed to
   * retain its value for "some time" after being returned. Particularly,
   * modifications to the Setting or accessing enough other Settings to
   * swap this Setting to disk will destroy the string.
   */
  class Setting {
    Setting(); ///< Not implemented
    Setting(const Setting&); ///<Not implemented
    Setting& operator=(const Setting&); ///<Not implemented

    public:
    /** Represents the possible types of a Setting. */
    enum Type {
      TypeInt=0,        ///<32-bit signed or unsigned integer
      TypeInt64,        ///<64-bit signed or unsigned integer
      TypeFloat,        ///<floating-point value (not stored as double)
      TypeString,       ///<8-bit character string
      TypeBoolean,      ///<bool type
      TypeArray,        ///<composite of anonymous homogeneous scalars
      TypeList,         ///<composite of anonymous hetrogeneous scalars and composites
      TypeGroup         ///<composite of named hetrogeneous scalars and composites
    };

    /** Returns the Setting as a boolean.
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeBoolean
     */
    operator bool() const;
    /** Returns the Setting as a signed integer.
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeInt
     */
    operator signed() const;
    /** Returns the Setting as an unsigned integer.
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeInt
     */
    operator unsigned() const;
    /** Returns the Setting as a signed long long.
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeInt64
     */
    operator signed long long() const;
    /** Returns the Setting as an unsigned long long.
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeInt64
     */
    operator unsigned long long() const;
    /** Returns the Setting as a float.
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TyeFloat
     */
    operator float() const;
    /** Returns the Setting as a const char*.
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeString
     */
    operator const char*() const;

    /** Alters the boolean value of this Setting.
     *
     * Complexity: O(1)
     * @return *this
     * @throw SettingTypeException if the Setting is not TypeBoolean
     */
    Setting& operator=(bool);
    /** Alters the signed integer value of this Setting.
     *
     * Complexity: O(1)
     * @return *this
     * @throw SettingTypeException if the Setting is not TypeInt
     */
    Setting& operator=(signed);
    /** Alters the unsigned integer value of this Setting.
     *
     * Complexity: O(1)
     * @return *this
     * @throw SettingTypeException if the Setting is not TypeInt
     */
    Setting& operator=(unsigned);
    /** Alters the signed long long value of this Setting.
     *
     * Complexity: O(1)
     * @return *this
     * @throw SettingTypeException if the Setting is not TypeInt64
     */
    Setting& operator=(signed long long);
    /** Alters the unsigned long long value of this Setting.
     *
     * Complexity: O(1)
     * @return *this
     * @throw SettingTypeException if the Setting is not TypeInt64
     */
    Setting& operator=(unsigned long long);
    /** Alters the float value of this Setting.
     *
     * Complexity: O(1)
     * @return *this
     * @throw SettingTypeException if the Setting is not TypeFloat
     */
    Setting& operator=(float);
    /** Alters the string value of this Setting.
     *
     * Complexity: O(n), n = strlen(v)
     * @arg v Input string to copy as the new value
     * @return *this
     * @throw SettingTypeException if the Setting is not TypeString
     */
    Setting& operator=(const char* v);
    /** Alters the string value of this Setting.
     *
     * Complexity: O(n), n = v.size()
     * @arg v Input string to copy as the new value
     * @return *this
     * @throw SettingTypeException if the Setting is not TypeString
     */
    Setting& operator=(const std::string& v);

    /** Accesses the child Setting at the given index.
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeGroup, TypeList, or TypeArray
     * @throw SettingNotFoundException if the index is out of range
     */
    Setting& operator[](unsigned);
    /** Accesses the child Setting at the given index.
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeGroup, TypeList, or TypeArray
     * @throw SettingNotFoundException if the index is out of range
     */
    const Setting& operator[](unsigned) const;
    /** Accesses the child Setting at the given index.
     *
     * This version exists because the compiler gets confused when a literal 0 is used
     * (which could indicate NULL for the const char* version or unsigned zero otherwise).
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeGroup, TypeList, or TypeArray
     * @throw SettingNotFoundException if the index is out of range
     */
    Setting& operator[](int i) { return operator[]((unsigned)i); }
    /** Accesses the child Setting at the given index.
     *
     * This version exists because the compiler gets confused when a literal 0 is used
     * (which could indicate NULL for the const char* version or unsigned zero otherwise).
     *
     * Complexity: O(1)
     * @throw SettingTypeException if the Setting is not TypeGroup, TypeList, or TypeArray
     * @throw SettingNotFoundException if the index is out of range
     */
    const Setting& operator[](int i) const { return operator[]((unsigned)i); }

    /** Accesses the child Setting with the given name.
     *
     * Complexity: O(1) average, O(n) worst
     * @throw SettingTypeException if the Setting is not TypeGroup
     * @throw SettingNotFoundException if no child with that name exists
     */
    Setting& operator[](const char*);
    /** Accesses the child Setting with the given name.
     *
     * Complexity: O(1) average, O(n) worst
     * @throw SettingTypeException if the Setting is not TypeGroup
     * @throw SettingNotFoundException if no child with that name exists
     */
    const Setting& operator[](const char*) const;
    /** Accesses the child Setting with the given name.
     *
     * Complexity: O(1) average, O(n) worst
     * @throw SettingTypeException if the Setting is not TypeGroup
     * @throw SettingNotFoundException if no child with that name exists
     */
    Setting& operator[](const std::string&);
    /** Accesses the child Setting with the given name.
     *
     * Complexity: O(1) average, O(n) worst
     * @throw SettingTypeException if the Setting is not TypeGroup
     * @throw SettingNotFoundException if no child with that name exists
     */
    const Setting& operator[](const std::string&) const;

    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const char* name, bool& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const std::string& name, bool& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const char* name, signed& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const std::string& name, signed& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const char* name, unsigned& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const std::string& name, unsigned& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const char* name, signed long long& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const std::string& name, signed long long& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const char* name, unsigned long long& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const std::string& name, unsigned long long& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const char* name, float& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const std::string& name, float& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const char* name, const char*& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const std::string& name, const char*& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const char* name, std::string& dst) const;
    /** Attempts to retrieve the value of the given direct child setting.
     *
     * If the access fails for any reason, no action is taken and no exception is thrown.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the child (directly) subordinate to this Setting
     * @arg dst The variable to store the value into; its type is used to determine the
     *   expected type of the Setting. On failure, it is unmodified.
     * @return Whether the operation succeeded
     */
    bool lookupValue(const std::string& name, std::string& dst) const;

    /** Adds a new child setting to this Setting.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the new child
     * @arg type The type of the new child
     * @return The new Setting
     * @throw SettingTypeException if this Setting is not of TypeGroup
     * @throw SettingNameException if the name is already in use or is invalid
     */
    Setting& add(const char* name, Type type);
    /** Adds a new child setting to this Setting.
     *
     * Complexity: O(1) average, O(n) worst
     * @arg name The name of the new child
     * @arg type The type of the new child
     * @return The new Setting
     * @throw SettingTypeException if this Setting is not of TypeGroup
     * @throw SettingNameException if the name is already in use or is invalid
     */
    Setting& add(const std::string& name, Type type);

    /** Adds a new child setting to the end of this Setting.
     *
     * Complexity: O(1) average
     * @arg type The type of the new child
     * @return The new Setting
     * @throw SettingTypeException if this Setting is not of TypeList or TypeArray,
     *   or if this Setting is of TypeArray and type is non-scalar or does not match
     *   the type of the other Settings contained in the array.
     */
    Setting& add(Type type);

    /** Removes the child Setting of the given name.
     *
     * Complexity: O(n)
     * @param name The name of the setting to remove
     * @throw SettingTypeException if this Setting is not of TypeGroup
     * @throw SettingNotFoundException if no such Setting exists
     */
    void remove(const char* name);
    /** Removes the child Setting of the given name.
     *
     * Complexity: O(n)
     * @param name The name of the setting to remove
     * @throw SettingTypeException if this Setting is not of TypeGroup
     * @throw SettingNotFoundException if no such Setting exists
     */
    void remove(const std::string& name);
    /** Removes the child Setting at the given index.
     *
     * Complexity: O(n)
     * @param ix The index of the child to remove
     * @throw SettingTypeException if this Setting is not of TypeGroup, TypeList, or TypeArray
     * @throw SettingNotFoundException if the index is out of range
     */
    void remove(unsigned ix);

    /** Returns the name of this Setting as known by its parent.
     *
     * The root returns NULL.
     *
     * Complexity: O(n), n = size of parent
     */
    const char* getName() const;
    /** Returns the complete path from the root to this Setting.
     *
     * Complexity: O(n*m), n = average size of parents, m = depth
     */
    std::string getPath() const;
    /** Returns the parent to this Setting.
     *
     * Complexity: O(1)
     * @throw SettingNotFoundException if this is the root.
     */
    Setting& getParent();
    /** Returns the parent to this Setting.
     *
     * Complexity: O(1)
     *
     * @throw SettingNotFoundException if this is the root.
     */
    const Setting& getParent() const;
    /** Return the parent to this Setting, or NULL if it is the root.
     *
     * Complexity: O(1)
     */
    Setting* getParentNoThrow();
    /** Return the parent to this Setting, or NULL if it is the root.
     *
     * Complexity: O(1)
     */
    const Setting* getParentNoThrow() const;

    /** Returns whether this Setting is the root. */
    bool isRoot() const;

    /** Returns the index of this Setting in its parent.
     *
     * The root returns ((unsigned)-1)
     *
     * Complexity: O(n)
     */
    unsigned getIndex() const;

    /** Returns the Type of this Setting. */
    Type getType() const;

    /** Returns whether a direct child of the given name exists.
     *
     * Never throws an exception.
     *
     * Complexity: O(1) average, O(n) worst
     */
    bool exists(const char*) const;
    /** Returns whether a direct child of the given name exists.
     *
     * Never throws an exception.
     *
     * Complexity: O(1) average, O(n) worst
     */
    bool exists(const std::string&) const;

    /** Returns the number of children this Setting has.
     *
     * For scalars, returns zero.
     *
     * Complexity: O(1)
     */
    unsigned getLength() const;

    /** Equivalent to getType()==TypeGroup */
    bool isGroup() const;
    /** Equivalent to getType()==TypeArray */
    bool isArray() const;
    /** Equivalent to getType()==TypeList */
    bool isList() const;

    /** Returns whether this Setting is of TypeGroup, TypeList, or TypeArray.
     *
     * Equivalent of isGroup() || isArray() || isList()
     */
    bool isAggregate() const;
    /** Equivalent of !isAggregate() */
    bool isScalar() const;
    /** Returns whether this Setting is of TypeInt, TypeInt64, or TypeFloat */
    bool isNumber() const;

    /** Returns the empty string (for compatibility). */
    const char* getSourceFile() const { static char ch=0; return &ch; }
    /** Returns 0 (for compatibility). */
    unsigned getSourceLine() const { return 0; }
  };

  /** Initialises the swapfile for spillover Settings.
   *
   * This MUST be called before used memory exceeds the primary storage limit.
   * The method of opening the file, as well as the name, location, and nature
   * of the file is platform-specific, but it will never be in Abendstern's primary
   * directory.
   *
   * @return NULL on success, error message on failure.
   */
  const char* openSwapFile();
  /** Destroys the swapfile.
   *
   * This should be called immediately before the application exits. This call will
   * NOT properly clean the config system up; <b>after this call, the config system
   * will be inoperable.</b>
   */
  void destroySwapFile();

  /** Returns the current maximum primary storage, in bytes.
   *
   * When allocation exceeds this amount, Settings will be moved into secondary
   * storage (the swap file) on a least-recently-used basis. Config objects, as
   * well as top-level indices cannot be swapped out and do not count toward
   * the storage usage amount.
   *
   * The default value is one megabyte.
   */
  std::size_t getMaxPriStorage();
  /** Sets the maximum primary storage, in bytes.
   *
   * @see getMaxPriStorage()
   */
  void setMaxPriStorage(std::size_t);

  /** Returns the current amount of primary storage in use, in bytes.
   *
   * @see getMaxPriStorage()
   */
  std::size_t getCurrPriStorage();

  /** Returns the current amount of secondary storage in use, in bytes.
   *
   * @see getMaxPriStorage()
   */
  unsigned long long getCurrSecStorage();

  /** Returns the current size of the swap file, in bytes.
   *
   * @see getMaxPriStorage()
   */
  unsigned long long getSwapFileSize();

  /** Deletes up to 1024 Settings queued for deletion and returns.
   *
   * This must be called to reclaim memory used by Settings.
   *
   * This system exists due to the fact that Abendstern in a number of
   * cases progressively creates an enormous number of Settings, then
   * discards them all simultaneously (eg, the ship editor and its numerous
   * undo states). An immediate recursive deletion of all these would cause
   * an unacceptable delay (eg, up to <i>seconds</i>) comparable to the
   * Embarrassing Pause in older Java.
   */
  void garbageCollection();
}

#endif /* LIBCONFIG_HXX_ */
