#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.50])
AC_INIT([Abendstern], [0.0], [jasonlingle@yahoo.com])
AC_CONFIG_SRCDIR([src/globals.hxx])
AM_INIT_AUTOMAKE([1.11])
AM_SILENT_RULES(yes)
AM_CONFIG_HEADER([config.h])
# Disable Automake's braindead want to rerun ./configure if it thinks it
# needs to regenerate the entire build system
AM_MAINTAINER_MODE
PKG_PROG_PKG_CONFIG()

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [Enable debugging.]))
AC_ARG_ENABLE([profile], AS_HELP_STRING([--enable-profile], [Enable profiling.]))
AC_ARG_ENABLE([gl14], AS_HELP_STRING([--disable-gl14], [Don't build OpenGL 1.4 version.]))
AC_ARG_ENABLE([gl21], AS_HELP_STRING([--disable-gl21], [Don't build OpenGL 2.1 version.]))
AC_ARG_ENABLE([gl32], AS_HELP_STRING([--disable-gl32], [Don't build OpenGL 3.2 version.]))
AC_ARG_WITH([glew], AS_HELP_STRING([--with-glew], [Use GLEW to access GL extensions.]))
AS_IF([test "x$enable_debug" = "xyes"], [
  AB_DEBUG_FLAGS='-DDEBUG -O0 -g3'
], [
  AB_DEBUG_FLAGS='-DNDEBUG -O3'
])
AS_IF([test "x$enable_profile" = "xyes"], [
  AB_PROFILE_FLAGS='-DPROFILE -pg -g3'
], [
  AB_PROFILE_FLAGS=''
])
AS_IF([test "x$with_glew" = "xyes"], [
  PKG_CHECK_MODULES([GLEW], [glew])
], [
  GLEW_CFLAGS=-DGL_GLEXT_PROTOTYPES
  GLEW_LIBS=
])
AS_IF([test "x$enable_gl14" = "xno"], [
  AM_CONDITIONAL(ENABLE_GL14, false)
], [
  AM_CONDITIONAL(ENABLE_GL14, true)
])
AS_IF([test "x$enable_gl21" = "xno"], [
  AM_CONDITIONAL(ENABLE_GL21, false)
], [
  AM_CONDITIONAL(ENABLE_GL21, true)
])
AS_IF([test "x$enable_gl32" = "xno"], [
  AM_CONDITIONAL(ENABLE_GL32, false)
], [
  AM_CONDITIONAL(ENABLE_GL32, true)
])

CXXFLAGS="$CXXFLAGS $AB_DEBUG_FLAGS $AB_PROFILE_FLAGS -DTHREAD_LOCAL=__thread"
LDFLAGS="$LDFLAGS $AB_PROFILE_FLAGS"

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP

# Check tclsh version.
# Debian does (or at least used to) default to tclsh 8.4 even if 8.5 was installed
# (this when Tcl 8.5 was already stable), so check for that condition to tell
# the user how to handle the situation.
echo -n 'checking for tclsh (8.5)... '
if echo ['exit [catch {package require Tcl 8.5}]'] | tclsh; then
  echo 'yes'
else
  echo 'no'
  echo 'Your either have an obsolete version of Tcl or do not have it at all.'
  echo 'Make sure that Tcl 8.5 is installed.'
  if echo ['exit [catch {package require Tcl 8.4}]'] | tclsh \
  && echo ['exit [catch {package require Tcl 8.5}]'] | tclsh8.5; then
    echo 'It appears that you do have Tcl 8.5 installed, but tclsh still links to Tcl 8.4.'
    echo 'Most likely, you are on Debian, or a derivitave such as Ubuntu.'
    echo 'Try running'
    echo '  sudo update-alternatives --config tclsh'
    echo 'and select tclsh8.5.'
  fi
  exit 1
fi

# Check for Itcl
echo -n ['checking for [incr Tcl]... ']
if echo ['exit [catch {package require Itcl}]'] | tclsh; then
  echo 'yes'
else
  echo 'no'
  echo ['Tcl cannot find the [incr Tcl] (also known as Itcl) library.']
  echo 'Make sure that Itcl (at least version 3.4) is installed.'
  exit 1
fi

# Check for required Tcllib libraries
# (sha256, aes, uuid)
for comp in sha256 aes uuid; do
  echo -n ["checking for Tcllib component $comp... "]
  if echo ["exit [catch {package require $comp}]"] | tclsh; then
    echo 'yes'
  else
    echo 'no'
    echo ["Tcl cannot find the $comp package, part of Tcllib."]
    echo 'Make sure that tcllib is installed.'
    exit 1
  fi
done

# Check for Tcl TLS library
echo -n ['checking for Tcl TLS... ']
if echo ['exit [catch {package require tls}]'] | tclsh; then
  echo 'yes'
else
  echo 'no'
  echo ['Tcl cannot find the tls package.']
  echo 'Make sure that tcltls (or tcl-tls) is installed.'
  exit 1
fi

# Don't need to check for http, it is part of standard Tcl 8.5

# Checks for libraries.
AC_LANG_PUSH([C++])
CPPFLAGS="$CPPFLAGS -I/usr/include/tcl8.5"
PKG_CHECK_MODULES([SDL], [sdl])
PKG_CHECK_MODULES([SDLIMG], [SDL_image])
PKG_CHECK_MODULES([GL], [gl])
PKG_CHECK_MODULES([PNG], [libpng])
AC_SEARCH_LIBS(Tcl_Init, [tcl8.5], [],
[AC_MSG_ERROR([unable to locate a Tcl library])])
AC_SEARCH_LIBS(Itcl_Init, [itcl3.4], [],
[AC_MSG_ERROR([unable to locate an Itcl library])])
AC_SEARCH_LIBS(__gmpz_powm, [gmp mpir], [],
[AC_MSG_ERROR([unable to locate a GMP or MPIR library])])
AC_CHECK_HEADER([asio.hpp], [], [AC_MSG_ERROR([could not find asio.hpp])])
AC_CHECK_HEADER([tcl.h], [], [AC_MSG_ERROR([could not find tcl.h])])
AC_CHECK_HEADER([itcl.h], [], [AC_MSG_ERROR([could not find itcl.h])])
AC_CHECK_HEADER([GL/gl.h], [], [AC_MSG_ERROR([could not find gl.h])])
AC_CHECK_HEADER([gmp.h], [], [AC_MSG_ERROR([could not find gmp.h])])
AC_CHECK_HEADER([boost/shared_array.hpp], [], [AC_MSG_ERROR([could not find boost/shared_array.hpp])])
AC_CHECK_HEADER([png++/png.hpp], [], [AC_MSG_ERROR([Could not find png++ header.])])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h malloc.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([atexit gettimeofday memmove memset pow setlocale socket sqrt strchr strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
