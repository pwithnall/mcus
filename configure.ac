AC_INIT([MCUS],[0.2.2],[http://tecnocode.co.uk/projects/mcus/],[mcus],[http://tecnocode.co.uk/projects/mcus/])

AM_INIT_AUTOMAKE([1.9 dist-bzip2 no-dist-gzip check-news subdir-objects])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS
AM_MAINTAINER_MODE
AC_PROG_CC
LT_INIT
PKG_PROG_PKG_CONFIG
AC_HEADER_STDC

# Enable silent build when available (Automake 1.11)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl ***************************************************************************
dnl Options
dnl ***************************************************************************

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug],[turn on debugging]),
	[case "${enableval}" in
		yes) debug=true ;;
		no)  debug=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
	esac],[debug=false])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])

dnl ***************************************************************************
dnl Windows support
dnl ***************************************************************************

case "${host}" in
        i[[3456789]]86-*-mingw32*) WIN32="yes" ;;
        *cygwin*) WIN32="yes" ;;
        *) WIN32="no" ;;
esac
AM_CONDITIONAL([WIN32], test "$WIN32" = "yes")

dnl Look for windres to build a Windows icon resource
AC_CHECK_TOOL([WINDRES], [windres], [no])
AM_CONDITIONAL([WITH_WIN_ICON], [test "$WINDRES" != "no"])

dnl ***************************************************************************
dnl Internationalisation
dnl ***************************************************************************

GETTEXT_PACKAGE=mcus
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

dnl ***************************************************************************
dnl Documentation
dnl ***************************************************************************

GNOME_DOC_INIT

dnl ***************************************************************************
dnl Compilation
dnl ***************************************************************************

PKG_CHECK_MODULES(STANDARD, glib-2.0 gtk+-2.0 >= 2.14 gmodule-2.0 gtksourceview-2.0 gthread-2.0)
AC_SUBST(STANDARD_CFLAGS)
AC_SUBST(STANDARD_LIBS)

AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
examples/Makefile
help/Makefile
])
AC_OUTPUT
