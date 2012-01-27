/**
 * @file
 * @author Jason Lingle
 *
 * @brief Defines a number of symbolic constants to indicate program exit status.
 */

#ifndef EXIT_CONDITIONS_HXX_
#define EXIT_CONDITIONS_HXX_

/** Indicates successful run of program and normal termination. */
#define EXIT_NORMAL             0
/** Indicates abnormal termination for unspecified reasons. */
#define EXIT_ABEND              1
/** Indicates that it appears that Abendstern cannot run on the platform. */
#define EXIT_PLATFORM_ERROR     2
/** Indicates the program is exiting due to a self-detected bug within C++. */
#define EXIT_PROGRAM_BUG        3
/** Indicates that an error occurred while executing a Tcl script and the program cannot continue. */
#define EXIT_SCRIPTING_BUG      4
/** Indicates that required data could not be understood. */
#define EXIT_MALFORMED_DATA     5
/** Indicates that required configuration information is missing or invalid. */
#define EXIT_MALFORMED_CONFIG   6
/** Indicates an exit for reasons supposed to be impossible. */
#define EXIT_THE_SKY_IS_FALLING 255

#endif /* EXIT_CONDITIONS_HXX_ */
