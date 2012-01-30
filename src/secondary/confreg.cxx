/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/secondary/confreg.hxx
 */

#include <libconfig.h++>
#include <map>
#include <string>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <cstdlib>

#include "confreg.hxx"
#include "src/exit_conditions.hxx"
#include "src/globals.hxx"

using namespace libconfig;
using namespace std;

ConfReg::~ConfReg() {
  for (iterator it=begin(); it != end(); ++it)
    delete (*it).second.conf;
}

void ConfReg::open(const string& filename, const string& key) {
  create(filename, key);
  ConfigRegEntry& ent(map<string,ConfigRegEntry>::operator[](key));
  ent.conf->readFile(filename.c_str());
}

void ConfReg::create(const string& filename, const string& key) {
  iterator it=find(key);
  if (it == end()) {
    ConfigRegEntry ent;
    ent.filename=filename;
    ent.modified=false;
    ent.conf=new Config;
    map<string,ConfigRegEntry>::operator[](key)=ent;
  } else {
    ConfigRegEntry& ent((*it).second);
    if (ent.modified) ent.conf->writeFile(ent.filename.c_str());
    ent.filename=filename;
    ent.modified=false;
  }
}

void ConfReg::close(const string& key) {
  iterator it=find(key);
  if (it != end()) {
    ConfigRegEntry& ent((*it).second);
    if (ent.modified && !ent.filename.empty()) ent.conf->writeFile(ent.filename.c_str());
    delete ent.conf;
    erase(it);
  }
}

void ConfReg::closeAll() {
  iterator it=begin();
  while (it != end()) {
    ConfigRegEntry& ent((*it).second);
    if (ent.modified && !ent.filename.empty()) ent.conf->writeFile(ent.filename.c_str());
    delete ent.conf;
    ++it;
  }
  clear();
}

//As best I can tell, the constructor for SettingNotFoundException
//is not supposed to be used externaly, so we need to simulate one
#define throwSettingNotFoundException(key) \
    Config temp; \
    temp.getRoot()[key].operator bool()

Setting& ConfReg::operator[](const string& key) {
  iterator it=find(key);
  if (it == end()) {
    throwSettingNotFoundException(key);
  }
  return (*it).second.conf->getRoot();
}

void ConfReg::modify(const string& key) {
  iterator it=find(key);
  if (it == end()) {
    throwSettingNotFoundException(key);
  }
  (*it).second.modified=true;
}

void ConfReg::unmodify(const string& key) {
  iterator it=find(key);
  if (it == end()) {
    throwSettingNotFoundException(key);
  }
  (*it).second.modified=false;
}

void ConfReg::sync(const string& key) {
  iterator it=find(key);
  if (it == end()) {
    throwSettingNotFoundException(key);
  }
  ConfigRegEntry& ent((*it).second);
  if (ent.modified && !ent.filename.empty()) ent.conf->writeFile(ent.filename.c_str());
  ent.modified=false;
}

void ConfReg::syncAll() {
  iterator it=begin();
  while (it != end()) {
    ConfigRegEntry& ent((*it).second);
    if (ent.modified && !ent.filename.empty()) ent.conf->writeFile(ent.filename.c_str());
    ent.modified=false;
    ++it;
  }
}

void ConfReg::revert(const string& key) {
  iterator it=find(key);
  if (it == end()) {
    throwSettingNotFoundException(key);
  }
  ConfigRegEntry& ent((*it).second);
  if (ent.filename.empty()) return;
  ent.conf->readFile(ent.filename.c_str());
  ent.modified=false;
}

void ConfReg::revertAll() {
  for (iterator it=begin(); it!=end(); ++it) {
    ConfigRegEntry& ent((*it).second);
    if (ent.filename.empty()) continue;
    ent.conf->readFile(ent.filename.c_str());
    ent.modified=false;
  }
}

void ConfReg::renameFile(const string& key, const string& filename) {
  iterator it=find(key);
  if (it == end()) {
    throwSettingNotFoundException(key);
  }
  ConfigRegEntry& ent(it->second);
  ent.filename = filename;
  ent.modified=true;
}

Setting& ConfReg::lookup(const string& path, bool mod) {
  if (!path.size()) throw runtime_error("Null path specified");
  whitelistCheck(path);

  size_t index=path.find('.');
  if (index == string::npos) {
    //Just the mountpoint
    iterator it=find(path);
    if (it==end()) {
      throw NoSuchSettingException(path.c_str());
    }
    if (mod) modify(path);
    return (*it).second.conf->getRoot();
  } else {
    string mount=path.substr(0, index);
    string sub=path.substr(index+1, string::npos);
    iterator it=find(mount);
    if (it==end()) {
      throw NoSuchSettingException(path.c_str());
    }
    if (mod) modify(mount);
    try {
      return (*it).second.conf->lookup(sub);
    } catch (SettingNotFoundException&) {
      throw NoSuchSettingException(path.c_str());
    }
  }

  cerr << "FATAL: We should never get here: "__FILE__": " << __LINE__ << endl;
  exit(EXIT_PROGRAM_BUG);
}

void ConfReg::whitelistCheck(const string& path) {
  if (!requireWhitelist) return;

  for (list<string>::const_iterator it=whitelist.begin();
       it != whitelist.end(); ++it)
    if (path.find_first_of(*it) == 0) return; //Starts with prefix

  throw NoSuchSettingException(path.c_str());
}

void ConfReg::addToWhitelist(const string& s) {
  whitelist.push_back(s);
}

void ConfReg::removeFromWhitelist(const string& s) {
  whitelist.remove(s);
}

void ConfReg::clearWhitelist() {
  whitelist.clear();
}

void ConfReg::setWhitelistOnly(bool b) {
  requireWhitelist=b;
}

bool ConfReg::exists(const string& path) {
  try {
    lookup(path);
    return true;
  } catch (...) {
    return false;
  }
}

bool ConfReg::getBool(const string& path) {
  return lookup(path);
}

int ConfReg::getInt(const string& path) {
  return lookup(path);
}

float ConfReg::getFloat(const string& path) {
  return lookup(path);
}

string ConfReg::getStr(const string& path) {
  return string(lookup(path).operator const char*());
}

void ConfReg::setBool(const string& path, bool b) {
  lookup(path, true)=b;
}

void ConfReg::setInt(const string& path, int i) {
  lookup(path, true)=i;
}

void ConfReg::setFloat(const string& path, float f) {
  lookup(path, true)=f;
}

void ConfReg::setString(const string& path, const string& s) {
  lookup(path, true)=s.c_str();
}

string ConfReg::getName(const string& path) {
  return string(lookup(path).getName());
}

void ConfReg::copy(const string& dst, const string& src) {
  confcpy(lookup(dst,true), lookup(src,false));
}

void ConfReg::add(const string& path, const string& child, Setting::Type type) {
  lookup(path, true).add(child.c_str(), type);
}

void ConfReg::addInt(const string& path, const string& child, int i) {
  lookup(path, true).add(child.c_str(), Setting::TypeInt)=i;
}

void ConfReg::addBool(const string& path, const string& child, bool b) {
  lookup(path, true).add(child.c_str(), Setting::TypeBoolean)=b;
}

void ConfReg::addFloat(const string& path, const string& child, float f) {
  lookup(path, true).add(child.c_str(), Setting::TypeFloat)=f;
}

void ConfReg::addString(const string& path, const string& child, const string& s) {
  lookup(path, true).add(child.c_str(), Setting::TypeString)=s;
}

void ConfReg::remove(const string& path) {
  size_t index=path.rfind('.');
  if (index==string::npos) throw runtime_error("Attempt to \"remove\" mountpoint");
  string parent=path.substr(0, index);
  string child=path.substr(index+1, string::npos);
  lookup(parent, true).remove(child.c_str());
}

void ConfReg::pushBack(const string& path, Setting::Type t) {
  lookup(path, true).add(t);
}

void ConfReg::pushBackBool(const string& path, bool b) {
  lookup(path, true).add(Setting::TypeBoolean)=b;
}

void ConfReg::pushBackInt(const string& path, int i) {
  lookup(path, true).add(Setting::TypeInt)=i;
}

void ConfReg::pushBackFloat(const string& path, float f) {
  lookup(path, true).add(Setting::TypeFloat)=f;
}

void ConfReg::pushBackString(const string& path, const string& s) {
  lookup(path, true).add(Setting::TypeString)=s;
}

void ConfReg::remix(const string& path, unsigned ix) {
  lookup(path, true).remove(ix);
}

/* Copys the value of the source setting into the destination setting.
 * It is assumed that the destination is already the right type and
 * is empty if non-scalar.
 */
void confcpy(Setting& dst, const Setting& src) {
  switch (src.getType()) {
    case Setting::TypeInt:
      dst = src.operator int();
      break;
    case Setting::TypeInt64:
      dst = src.operator long long();
      break;
    case Setting::TypeFloat:
      dst = src.operator float();
      break;
    case Setting::TypeString:
      dst = src.operator const char*();
      break;
    case Setting::TypeBoolean:
      dst = src.operator bool();
      break;
    case Setting::TypeArray:
    case Setting::TypeList: {
      for (unsigned i=0; i<(unsigned)src.getLength(); ++i) {
        const Setting& subsrc(src[i]);
        Setting& subdst(dst.add(subsrc.getType()));
        confcpy(subdst, subsrc);
      }
    } break;
    case Setting::TypeGroup: {
      for (unsigned i=0; i<(unsigned)src.getLength(); ++i) {
        const Setting& subsrc(src[i]);
        const char* name=subsrc.getName();
        Setting& subdst=(dst.add(name, subsrc.getType()));
        confcpy(subdst, subsrc);
      }
    } break;
    default: {
      cerr << "FATAL: Unexpected config type: " << src.getType() << endl;
      exit(EXIT_PROGRAM_BUG);
    }
  }
}

void confcpy(const string& dst, const string& src) {
  confcpy(conf.lookup(dst, true), conf.lookup(src, false));
}

#define PROC(ret, nam) ret ConfReg::nam(const string& path) { return lookup(path).nam(); }
PROC(Setting::Type, getType)
PROC(int, getLength)
PROC(bool, isGroup)
PROC(bool, isArray)
PROC(bool, isList)
PROC(bool, isAggregate)
PROC(bool, isScalar)
PROC(bool, isNumber)
PROC(int, getSourceLine);
