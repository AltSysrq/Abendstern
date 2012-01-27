/**
 * @file
 * @author Jason Lingle
 * @brief Implementation of src/core/lxn.hxx
 */

/*
 * l10n.cxx
 *
 *  Created on: 24.06.2011
 *      Author: jason
 */

#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <cerrno>

#include "lxn.hxx"

using namespace std;

struct entry_t { string sourceLanguage, value; };
typedef map<string,entry_t> section_t;
typedef map<string,section_t> catalogue_t;

static catalogue_t catalogues[26];
static vector<string> acceptedLanguages;

void l10n::acceptLanguage(const char* lang) {
  acceptedLanguages.push_back(lang);
}

bool l10n::loadCatalogue(char cat, const char* filename) {
  if (cat < 'A' || cat > 'Z') {
    cerr << "Refusing to load catalogue " << cat << endl;
    return false;
  }

  unsigned cati = cat-'A';

  ifstream in(filename);
  if (!in) {
    cerr << "Unable to open " << filename << strerror(errno) << endl;
    return false;
  }

  string currLang, currSection;

  string line;
  unsigned lineNum = 0;
  bool currLangAccepted = false;
  entry_t* currEntry = NULL;

  while (getline(in, line)) {
    ++lineNum;

    //Strip CR in case the file was written on Windows
    if (!line.empty() && line[line.size()-1] == '\r')
      line.erase(line.begin() + (line.size()-1));

    //Skip ignored lines
    if (line.empty() || line[0] == ' ' || line[0] == '\t' || line[0] == '#')
      continue;

    switch (line[0]) {
      case '~':
        currLang = line.c_str()+1;
        currSection = "";
        currLangAccepted = false;
        currEntry = NULL;
        for (unsigned i=0; i<acceptedLanguages.size() && !currLangAccepted; ++i)
          currLangAccepted = (acceptedLanguages[i] == currLang);
        break;
      case '@':
        if (currLang.empty()) {
          cerr << filename << ':' << lineNum << ": Section without language" << endl;
          return false;
        }
        currSection = line.c_str()+1;
        currEntry = NULL;
        break;
      case '-':
        if (currLang.empty() || currSection.empty()) {
          cerr << filename << ':' << lineNum << ": Entry without section" << endl;
          return false;
        }

        if (currLangAccepted) {
          string ename = line.c_str()+1;
          //See if this entry already exists. If it doesn't, we do accept it
          //and create it. Otherwise, see whether the current language or
          //the entry's language comes first in the preferred list. If they
          //are at the same place, this is a duplicate, which is an error
          //condition. If the old comes first, we do not accept, and wait
          //for the next entry. If this comes first, we do accept.
          section_t& sec(catalogues[cati][currSection]);
          section_t::iterator it = sec.find(ename);
          if (it == sec.end()) {
            //Create
            currEntry = &sec[ename];
          } else {
            unsigned ix = 0;
            for (; ix < acceptedLanguages.size(); ++ix)
              if (acceptedLanguages[ix] == currLang
              ||  acceptedLanguages[ix] == it->second.sourceLanguage)
                break;
            //Since currLangAccepted, ix will ALWAYS be a valid index
            //after this point
            //First, check for duplicate
            if (acceptedLanguages[ix] == currLang
            &&  acceptedLanguages[ix] == it->second.sourceLanguage) {
              cerr << filename << ':' << lineNum << ": Entry " << cat << ',' << currSection << ',' << ename
                   << " already exists." << endl;
              return false;
            }

            //No error, do we accept?
            if (acceptedLanguages[ix] == currLang)
              currEntry = &it->second;
            else
              currEntry = NULL;
          }

          if (currEntry) {
            currEntry->sourceLanguage = currLang;
            currEntry->value = "";
          }
        }

        break;
      case '*':
        if (currEntry) currEntry->value += "\n";
        //fall-through
      case '+':
        if (currEntry) {
          string txt;
          txt.reserve(line.size()); //One added for term NUL
          //Process escape sequences
          for (unsigned i=1; i < line.size(); /* i updated in switch */ ) switch (line[i]) {
            case '\\':
              if (i+1 >= line.size()) {
                cerr << filename << ':' << lineNum << ": Trailing \\ at end of line" << endl;
                return false;
              }
              switch (line[++i]) {
                case '\\': txt += '\\'; ++i; break;
                case 'a':  txt += '\a'; ++i; break;
                case 'n':  txt += '\n'; ++i; break;
                case 't':  txt += '\t'; ++i; break;
                case 'x': {
                  if (i+2 >= line.size()) {
                    cerr << filename << ':' << lineNum << ": Incomplete \\x sequence at end of line" << endl;
                    return false;
                  }
                  char val = 0;
                  for (unsigned j=0; j<2; ++j) {
                    val <<= 4;
                    char ch = line[++i];
                    if (ch >= '0' && ch <= '9')
                      val |= (ch-'0');
                    else if (ch >= 'a' && ch <= 'f')
                      val |= (ch+10-'a');
                    else if (ch >= 'A' && ch <= 'F')
                      val |= (ch+10-'A');
                    else {
                      cerr << filename << ':' << lineNum << ": Invalid hex digits in \\x escape sequence" << endl;
                      return false;
                    }
                  }

                  txt += val;
                  ++i;
                } break;

                default:
                  cerr << filename << ':' << lineNum << ": Invalid escape sequence" << endl;
                  return false;
              }
              break;

            default:
              //Non-ASCII UTF-8 are negative on systems with signed chars
              if ((line[i] >= 0 && line[i] < ' ') || line[i] == 127) {
                cerr << filename << ':' << lineNum << ": Control characters in input" << endl;
                return false;
              }
              txt += line[i++];
              break;
          }

          currEntry->value += txt;
        }
        break;

      default:
        cerr << filename << ':' << lineNum << ": Unclassifiable line" << endl;
        return false;
    }
  }

  return true;
}

void l10n::purgeCatalogue(char cat) {
  if (cat >= 'A' && cat <= 'Z')
    catalogues[(unsigned)(cat-'A')].clear();
}

const std::string& l10n::lookup(char cat, const std::string& sec, const std::string& entr) {
  static const string empty, secNotFound("####"), entNotFound("#####");
  if (cat < 'A' || cat > 'Z') return empty;
  unsigned cati = cat-'A';

  catalogue_t::const_iterator cit = catalogues[cati].find(sec);
  if (cit == catalogues[cati].end()) {
    cerr << "l10n: Section not found: " << cat << ',' << sec << ',' << entr << endl;
    return secNotFound;
  }

  section_t::const_iterator sit = cit->second.find(entr);
  if (sit == cit->second.end()) {
    cerr << "l10n: Entry not found: " << cat << ',' << sec << ',' << entr << endl;
    return entNotFound;
  }

  return sit->second.value;
}

const char* l10n::lookup(char cat, const char* sec, const char* entr) {
  return lookup(cat, string(sec), string(entr)).c_str();
}
