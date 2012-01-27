/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/ship/insignia.hxx
 */

#include <cstring>
#include <map>
#include <vector>
#include <string>

#include "insignia.hxx"
using namespace std;

namespace insignia_items {
  //First key is always < second key
  typedef map<unsigned long, map<unsigned long, Alliance> > override_t;
  override_t overrides;
  vector<string> insignias;
}
using namespace insignia_items;

unsigned long insignia(const char* name) {
  if (0==strcmp(name, "neutral")) return 0;
  else for (unsigned int i=0; i<insignias.size(); ++i)
    if (0==strcmp(name, insignias[i].c_str())) return i+1;

  insignias.push_back(name);
  return insignias.size();
}

void clear_insignias() {
  overrides.clear();
  insignias.clear();
}

Alliance getAlliance(unsigned long a, unsigned long b) {
  if (!a || !b) return Neutral;
  if (a > b) { unsigned long c=a; a=b; b=c; }
  override_t::iterator ait=overrides.find(a);
  if (ait == overrides.end()) return a == b? Allies : Enemies;
  map<unsigned long,Alliance>::iterator bit=(*ait).second.find(b);
  if (bit == (*ait).second.end()) return a==b? Allies : Enemies;
  return (*bit).second;
}

void setAlliance(Alliance all, unsigned long a, unsigned long b) {
  if (a > b) { unsigned long c=a; a=b; b=c; }
  overrides[a][b]=all;
}
