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
AC_ARG_ENABLE([optimise], AS_HELP_STRING([--disable-optimise],
                                         [Disable optimisations]))
AC_ARG_ENABLE([profile], AS_HELP_STRING([--enable-profile], [Enable profiling.]))
AC_ARG_ENABLE([gl14], AS_HELP_STRING([--disable-gl14], [Don't build OpenGL 1.4 version.]))
AC_ARG_ENABLE([gl21], AS_HELP_STRING([--disable-gl21], [Don't build OpenGL 2.1 version.]))
AC_ARG_ENABLE([gl32], AS_HELP_STRING([--disable-gl32], [Don't build OpenGL 3.2 version.]))
AC_ARG_WITH([glew], AS_HELP_STRING([--with-glew], [Use GLEW to access GL extensions.]))
AS_IF([test "x$enable_debug" = "xyes"], [
  AB_DEBUG_FLAGS='-DDEBUG -g3'
], [
  AB_DEBUG_FLAGS='-DNDEBUG'
])
AS_IF([test "x$enable_optimise" = "xno"],
  AB_OPT_FLAGS='-O0'
], [
  AB_OPT_FLAGS='-O3'
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

AC_ARG_WITH([fonts],
  AS_HELP_STRING([--without-fonts],
    [Don't generate font files (useful for servers).]))
AS_IF([test "x$with_fonts" = "xno"], [
  AM_CONDITIONAL(ENABLE_FONTS, false)
], [
  AM_CONDITIONAL(ENABLE_FONTS, true)
])

CXXFLAGS="$CXXFLAGS $AB_DEBUG_FLAGS $AB_OPT_FLAGS $AB_PROFILE_FLAGS \
    -DTHREAD_LOCAL=__thread"
LDFLAGS="$LDFLAGS $AB_PROFILE_FLAGS"

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PATH_PROG([IMAGEMAGICK_CONVERT], [convert])
AS_IF([test "x$IMAGEMAGICK_CONVERT" = "x" -a "x$with_fonts" != "xno"], [
  echo 'The convert program (from ImageMagick) is required to generate the font'
  echo 'files, but was not found in your PATH.'
  echo 'Make sure ImageMagick is installed.'
  exit 1
])

# Check tclsh version.
# Check for tclsh8.5 first since some systems (eg, FreeBSD) don't even create a
# tclsh link.
AC_PATH_PROGS([TCL], [tclsh8.5 tclsh])
if test "x$TCL" == "x"; then
  echo 'No Tcl executable ("tclsh8.5" or "tclsh") could be found in your PATH.'
  echo 'Make sure Tcl 8.5 is installed.'
  exit 1
fi
# Ensure that the version is correct
echo -n 'checking Tcl version (8.5)... '
if echo ['exit [catch {package require Tcl 8.5}]'] | $TCL; then
  echo 'yes'
else
  echo 'no'
  echo 'Your either have an obsolete version of Tcl or do not have it at all.'
  echo 'Make sure that Tcl 8.5 is installed.'
  # This check is now obsolete since we use tclsh8.5 if it exists, in
  # preference to linked tclsh.
  #if echo ['exit [catch {package require Tcl 8.4}]'] | tclsh \
  #&& echo ['exit [catch {package require Tcl 8.5}]'] | tclsh8.5; then
    #echo 'It appears that you do have Tcl 8.5 installed, but tclsh still links to Tcl 8.4.'
    #echo 'Most likely, you are on Debian, or a derivitave such as Ubuntu.'
    #echo 'Try running'
    #echo '  sudo update-alternatives --config tclsh'
    #echo 'and select tclsh8.5.'
  #fi
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
#echo -n ['checking for Tcl TLS... ']
#if echo ['exit [catch {package require tls}]'] | tclsh; then
#  echo 'yes'
#else
#  echo 'no'
#  echo ['Tcl cannot find the tls package.']
#  echo 'Make sure that tcltls (or tcl-tls) is installed.'
#  exit 1
#fi

# Don't need to check for http, it is part of standard Tcl 8.5

# Checks for libraries.
AC_LANG_PUSH([C++])
CPPFLAGS="$CPPFLAGS -I/usr/include/tcl8.5 -I/usr/local/tcl8.5 -L/usr/lib/itcl3.4 -L/usr/local/lib/itcl3.4"
PKG_CHECK_MODULES([SDL], [sdl])
PKG_CHECK_MODULES([SDLIMG], [SDL_image])
PKG_CHECK_MODULES([GL], [gl])
PKG_CHECK_MODULES([PNG], [libpng])
AC_SEARCH_LIBS(Tcl_Init, [tcl8.5], [],
[AC_MSG_ERROR([unable to locate a Tcl library])])
#For whatever reason, some distros place itcl somewhere outside the lib path,
#so check everywhere it could be
AC_SEARCH_LIBS(Itcl_Init, [itcl3.4 ictl3.4/itcl3.4 itcl/itcl3.4 tcl8.5/itcl/itcl3.4 tcl8.5/itcl3.4/itcl3.4], [],
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

AC_CHECK_HEADERS([drachen.h])
AC_SEARCH_LIBS(drachen_encode, [drachen], [], [
  echo "Note: You do not seem to have libdrachen installed."
  echo "This is not a problem unless you want to record game footage."
])

AC_CHECK_HEADERS([arpa/inet.h inttypes.h malloc.h netinet/in.h stdint.h dnl
                  stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([atexit gettimeofday memmove memset pow setlocale socket sqrt strchr strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 fonts/Makefile
                 fonts/westm/Makefile
                 fonts/unifont/Makefile])
AC_OUTPUT
