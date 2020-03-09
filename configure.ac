AC_PREREQ(2.52)

AC_INIT(imcanna.c)

PACKAGE=im-canna
VERSION=0.3.4

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)

AC_PROG_CC
AM_DISABLE_STATIC
AM_PROG_LIBTOOL

ALL_LINGUAS="ja"
AC_SUBST(ALL_LINGUAS)

GETTEXT_PACKAGE=im-canna
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AM_GLIB_GNU_GETTEXT

GTK_DEP_CFLAGS=`pkg-config gtk+-2.0 --cflags`
GTK_DEP_LIBS=`pkg-config gtk+-2.0 --libs`
GTK_VERSION=`pkg-config gtk+-2.0 --variable=gtk_binary_version`
AC_SUBST(GTK_DEP_CFLAGS)
AC_SUBST(GTK_DEP_LIBS)
AC_SUBST(GTK_VERSION)

LT_RELEASE=$GTK_MAJOR_VERSION.$GTK_MINOR_VERSION
LT_CURRENT=2
LT_REVISION=$GTK_INTERFACE_AGE
LT_AGE=2
LT_CURRENT_MINUS_AGE=0
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_CURRENT_MINUS_AGE)

AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

dnl Canna check
AC_CHECK_HEADER(canna/jrkanji.h,,AC_MSG_ERROR(No Canna header))
AC_CHECK_LIB(canna, jrKanjiString,, AC_MSG_ERROR(No Canna library))

AC_OUTPUT([
Makefile
im-canna.spec
po/Makefile.in
])
