/**
 * @file
 * @author Jason Lingle
 * @brief Contains Abendstern's localisation system
 */

/*
 * l10n.hxx
 *
 *  Created on: 20.06.2011
 *      Author: jason
 */

#ifndef L10N_HXX_
#define L10N_HXX_

#include <string>

/**
 * @brief Localisation functions.
 *
 * \verbatim
 * Abendstern's localisation system provides a facility similar to
 * gettext, except that items are given symbolic names to reduce
 * fragility of the translations. Also unlike gettext, it is not joined
 * at the hip with autotools, and it only uses the C++ standard
 * library.
 *
 * The message catalogue is loaded from one or more files of a format
 * described below. Each describes a number of names with corresponding
 * texts; each file's contents are placed under a single top-level catalogue
 * entry, typically the basename of the file. Each file defines any number
 * of named sections, each section containing any number of name/text pairs.
 *
 * The files are assumed to be encoded in UTF-8. Empty lines are ignored.
 * The first character of each line determines its meaning:
 *   #, space   Comment (line ignored)
 *   ~          Language; rest of line defines language for the rest of
 *              the file, or the next ~. If the specified language is
 *              not to be loaded, that part of the file is ignored.
 *   @          New section; rest of line is section name
 *   -          New entry; rest of line is entry name
 *   +          New text; rest of line is appended to current entry text
 *   *          New text preceeded by newline
 *
 * Within text, a number of escape sequences may be used:
 *   \\         Single backslash
 *   \a         Alert
 *   \n         Newline
 *   \t         Tabstop
 *   \xXX       Single hexadecimal byte XX
 *
 * The following are error conditions:
 *   Non-comment commands before any ~ command
 *   Control characters anywhere in input (other \n or \r) (0..31, 127)
 *   Unknown start-of-line sequence
 *   Unknown \-escape sequence
 *   Trailing lone \
 *
 * When fetching a string, the loaded languages are searched in preferential
 * order until a match is found. If none is found, the key is returned, and
 * an error printed to stderr.
 * \endverbatim
 */
namespace l10n {
  /** Adds the specified language to the list of languages to use. It will be
   * the final one in the preferential list.
   *
   * @param lang The code of the language to add
   */
  void acceptLanguage(const char* lang);

  /** Loads a catalogue entry for the current languages.
   * On failure, prints a diagnostic to stderr and returns false. However,
   * any modifications already made by the time an error is encountered
   * will persist (ie, this is not an atomic operation).
   *
   * @param cat The catalogue to load, a single uppercase ASCII letter
   * @param filename The filename from where to load the catalogue
   * @return True on success, false on failure
   * @see Namespace l10n
   */
  bool loadCatalogue(char cat, const char* filename);

  /** Purges a catalogue entry that was formerly loaded.
   * If it does not currently exist, nothing happens.
   */
  void purgeCatalogue(char);

  /** Looks the given key up and returns the best match. */
  const char* lookup(char,const char*,const char*);
  /** Looks the given key up and returns the best match. */
  const std::string& lookup(char, const std::string&, const std::string&);
}

/**
 * This macro handles the common case of A, <literal>, <literal>.
 * All non-testing Abendstern messages are under A.
 * It is used like<br>
 * &nbsp;  <code>_(engine_mounting_req,particle_accelerator)</code><br>
 * Macro substitiution on the parms is not performed.
 */
#define _(x,y) l10n::lookup('A',#x,#y)

/** Macro to look a given entry up exactly one time, then set the specified
 * variable every time.
 */
#define SL10N(var,x,y) { static std::string var##_static = l10n::lookup('A', #x, #y); var = var##_static.c_str(); }

/** Macro to return a const char* message (ie, return from calling function). */
#define RETL10N(x,y) { static std::string l10nreturnval = l10n::lookup('A', #x, #y); return l10nreturnval.c_str(); }

#endif /* L10N_HXX_ */
