/**
 * @file
 * @author Jason Lingle
 * @brief Contains facilities for automatic name generation for AI, etc.
 *
 * Files under data/namegen are plaintext files in UTF-8 that list names
 * (or words) used in a certain region. For example, data/namegen/enbc
 * contains the 1442 most-given names in British Colombia. Names are
 * separated by LFs (CRs will end up being part of the name except on
 * Windows --- files MUST be encoded with LF only!). After separation,
 * the names are pseudo-decoded from UTF-8 into arrays of integers (this
 * makes the assumption that no character we will ever encounter will span
 * more than four bytes). This allows multibyte characters to be handled
 * correctly and flexibly.
 *
 * To generate a name, the generator selects a random name from the chosen
 * namegroup and takes its first two characters. It then applies the
 * Dissociated Press algorithm until a NUL character is emitted. The
 * result is then transcoded back to UTF-8 and returned.
 */

/*
 * namegen.hxx
 *
 *  Created on: 22.07.2011
 *      Author: jason
 */

#ifndef NAMEGEN_HXX_
#define NAMEGEN_HXX_

#include <string>

#include "src/opto_flags.hxx"

/** Loads the namegen files.
 * The list of known regions (and their weights for selecting at
 * random) can be found at data/namegen/list.dat.
 * This must be called before any other function in this file.
 *
 * On failure, aborts the program.
 */
void namegenLoad() noth;

/** Generates and returns a new name.
 *
 * @param region Name of the region to generate from. If NULL or the
 *   region does not exist, it is selected randomly by defined weights.
 */
std::string namegenGet(const char* region = NULL);

#endif /* NAMEGEN_HXX_ */
