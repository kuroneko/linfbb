AC_PREREQ([2.63])
AC_INIT(fbb, 7.0.9, f6bvp@free.fr)
AC_CONFIG_SRCDIR([src/xfbbd.c])
AC_CONFIG_HEADERS([config.h])

# We don't use "subdir-objects" yet because it breaks "make distclean" 
# and "make distcheck", and therefore ./autogen.sh when dependencies 
# are enabled (as of Automake 1.14) due to this bug:
# http://debbugs.gnu.org/cgi/bugreport.cgi?bug=17354
# The -Wno-unsupported is used to silence warnings about missing 
# "subdir-objects".
# Maybe we'll look at re-organizing things to be non-recursive, or 
# maybe we'll leave it for the time being.
AM_INIT_AUTOMAKE([-Wall -Werror dist-bzip2 -Wno-unsupported])
m4_include([m4/ax_with_curses.m4])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Determine versions for package managers.
basever=$(echo $PACKAGE_VERSION | sed -e 's@-.*$@@')
AC_SUBST(BASEVERSION, [$basever])
extraver=$(echo $PACKAGE_VERSION | sed -e 's@'$basever'@@' -e 's@^-@_@')
AC_SUBST(EXTRAVERSION, [$extraver])
SVNREV=`( svnversion -n $srcdir )`
if { ! ( svnversion ) >/dev/null 2>&1 || test "$SVNREV" = "exported"; } ;
        then SVNREV=`cat $srcdir/SVNREV`
        else echo $SVNREV>$srcdir/SVNREV
fi
AC_SUBST(SVNREV)

# Decompose the version number.
#
# We must do this rather than construct PACKAGE_VERSION from the components
# because AC_INIT rejects anything but a literal for the version number.
PACKAGE_VERSION_MAJOR=`echo "$PACKAGE_VERSION" | cut -d. -f1`
PACKAGE_VERSION_MINOR=`echo "$PACKAGE_VERSION" | cut -d. -f2`
PACKAGE_VERSION_MICRO=`echo "$PACKAGE_VERSION" | cut -d. -f3 | sed -e 's@-.*$@@'`

AC_SUBST([PACKAGE_VERSION_MAJOR])
AC_SUBST([PACKAGE_VERSION_MINOR])
AC_SUBST([PACKAGE_VERSION_MICRO])

AC_DEFINE_UNQUOTED(PACKAGE_VERSION_MAJOR, $PACKAGE_VERSION_MAJOR, [Define to the major version of this package.])
AC_DEFINE_UNQUOTED(PACKAGE_VERSION_MINOR, $PACKAGE_VERSION_MINOR, [Define to the minor version of this package.])
AC_DEFINE_UNQUOTED(PACKAGE_VERSION_MICRO, $PACKAGE_VERSION_MICRO, [Define to the micro version of this package.])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_MKDIR_P
AM_PROG_CC_C_O

# We want to test for the presence of all the necessary Xlibs.
# Default to building the X binaries, IF we have all the right 
# libraries involved. If we do not have all the necessary 
# libraries OR configure is run with --disable-x-utils, then 
# we won't build any of the X binaries.

# Check how configure is executed and see if we are forced to 
# not build the X binaries, otherwise default to yes.
AC_ARG_ENABLE([x-utils],
	[AS_HELP_STRING([--disable-x-utils],
	[disable building X windows utils @<:@default: no@:>@])],
	[x_utils=${enableval}], [x_utils=yes])

# Check for all the necessary X libraries.
# Use conditional tests for each library so we can stop 
# when we hit one that is missing. Note the first test is different 
# than the others, it checks to see if we are even building X utilities.
have_xlibs=no
if test "x${x_utils}" = xyes; then
	AC_CHECK_LIB([X11], [XOpenDisplay], [have_xlibs=yes], [have_xlibs=no])
fi
if test "x${have_xlibs}" = xyes; then
	AC_CHECK_LIB([Xext], [main], [have_xlibs=yes], [have_xlibs=no])
fi
if test "x${have_xlibs}" = xyes; then
AC_CHECK_LIB([Xm], [XmCreateMainWindow], [have_xlibs=yes], [have_xlibs=no])
fi
if test "x${have_xlibs}" = xyes; then
AC_CHECK_LIB([Xpm], [main], [have_xlibs=yes], [have_xlibs=no])
fi
if test "x${have_xlibs}" = xyes; then
AC_CHECK_LIB([Xt], [XtOpenDisplay], [have_xlibs=yes], [have_xlibs=no])
fi

# If we want to build the X binaries (default is yes), but we are 
# missing one of the required libraries, throw a warning but 
# continue to build without X support. The user can go back and 
# resolve the dependencies if they really want X.
if test "x${have_xlibs}" = xno; then
	if test "x${x_utils}" = xyes; then
		AC_MSG_WARN([
	----------------------------------
	Missing one of X11, Xext, Xm, Xpm
	or Xt libraries. Not building 
	X Windows utilities.
	
	If you want to build the X Windows 
	utilities, you need:
	libx11-dev (provides -lX11)
	libxext-dev (provides -lXext)
	lesstif2-dev (provides -lXm)
	libxpm-dev (provides -lXpm)
	libxt-dev (provides -lXt)
	Or the equivalent packages on your OS.
	----------------------------------])
	fi
	x_utils=no
fi

# If all the tests for X libraries pass, set a HAVE_X_LIBS conditional
# to allow us to conditionally build the sources in src/X11
AM_CONDITIONAL([HAVE_X_LIBS], [test "$x_utils" = yes])

# Check for libmath
AC_CHECK_LIB([m], [cos], [], [AC_MSG_ERROR([Could not find libmath; aborting])])

# Check for AX25
AC_CHECK_LIB([ax25], [ax25_config_load_ports], [AX25_LIB="-lax25"], [AC_MSG_ERROR(Could not find the libax25 libraries; aborting)])
AC_SUBST(AX25_LIB)

# Check for ncurses libraries
AC_CHECK_LIB(ncurses, initscr, NCURSES_LIB="-lncurses", AC_MSG_ERROR([Could not find the ncurses libraries; aborting]))

# Use an m4 macro to check for curses
AX_WITH_CURSES
case "$ax_cv_ncurses" in "no")
        AC_MSG_WARN([ncurses not found.])
        AC_MSG_WARN([xfbbC will be built with terminal mode.])
esac
AM_CONDITIONAL(HAVE_NCURSES, test x$ax_cv_ncurses = xyes -o x$ax_cv_ncursesw = xyes)
AC_SUBST(CURSES_LIB)

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h locale.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/file.h sys/ioctl.h sys/socket.h sys/time.h sys/vfs.h syslog.h termios.h unistd.h utime.h values.h limits.h stdio.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm dup2 floor getcwd gethostbyname gettimeofday inet_ntoa memmove memset mkdir modf perror pow putenv rmdir select setlocale socket sqrt strcasecmp strchr strdup strerror strncasecmp strndup strrchr strspn strstr tzset utime])

AC_CONFIG_FILES([Makefile doc/Makefile doc/html/Makefile etc/Makefile man/Makefile include/Makefile scripts/Makefile src/Makefile src/X11/Makefile])
AC_OUTPUT
