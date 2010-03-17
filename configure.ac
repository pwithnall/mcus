# Release version and URI (seems to not define PACKAGE_URL if put in AC_INIT)
m4_define(version_major, 0)
m4_define(version_minor, 3)
m4_define(version_micro, 0)

AC_INIT([MCUS],[version_major.version_minor.version_micro],[http://tecnocode.co.uk/projects/mcus/],[mcus],[http://tecnocode.co.uk/projects/mcus/])

AM_INIT_AUTOMAKE([1.9 dist-bzip2 no-dist-gzip check-news subdir-objects])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS
AM_MAINTAINER_MODE
AM_PROG_CC_C_O
LT_INIT
PKG_PROG_PKG_CONFIG
AC_HEADER_STDC

# Enable silent build when available (Automake 1.11)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Substitute the version
AC_SUBST(VERSION_MAJOR,version_major)
AC_SUBST(VERSION_MINOR,version_minor)
AC_SUBST(VERSION_MICRO,version_micro)

# AC_INIT doesn't seem to be defining PACKAGE_URL for us, so force it to
m4_ifndef([AC_PACKAGE_URL],[AC_SUBST([PACKAGE_URL],[http://tecnocode.co.uk/projects/mcus/])])
m4_ifndef([AC_PACKAGE_URL],[AC_DEFINE_UNQUOTED([PACKAGE_URL],["http://tecnocode.co.uk/projects/mcus/"],[Package website])])

dnl ***************************************************************************
dnl Options
dnl ***************************************************************************

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug],[turn on debugging]),
	[case "${enableval}" in
		yes) debug=yes ;;
		no)  debug=no ;;
		*) AC_MSG_ERROR([Bad value ${enableval} for --enable-debug.]) ;;
	esac],[debug=no])
AM_CONDITIONAL([DEBUG], [test "$debug" = "yes"])

dnl ***************************************************************************
dnl Windows support
dnl ***************************************************************************

case "${host}" in
        i[[3456789]]86-*-mingw32*) win32="yes" ;;
        *cygwin*) win32="yes" ;;
        *) win32="no" ;;
esac
AM_CONDITIONAL([WIN32], [test "$win32" = "yes"])

if test "$win32" = "yes"; then
	dnl Look for windres to build a Windows icon resource
	AC_CHECK_TOOL([WINDRES], [windres], [no])
	AC_ARG_VAR([WINDRES], [path to windres utility])
	AM_CONDITIONAL([WITH_WIN_ICON], [test "$WINDRES" != "no"])
	AC_CONFIG_FILES([data/icons/mcus_win_icon.rc])

	dnl Build the installer if possible
	AC_ARG_ENABLE([installer],
		AS_HELP_STRING([--enable-installer],[build a Windows NSIS installer]),
		[case "${enableval}" in
			yes) installer=yes ;;
			no)  installer=no ;;
			*) AC_MSG_ERROR([Bad value ${enableval} for --enable-installer.]) ;;
		esac],[installer=yes])

	AC_PATH_PROG([MAKENSIS], makensis, no)
	AC_ARG_VAR([MAKENSIS], [path to makensis NSIS package builder])
	if test "$MAKENSIS" = "no" -a "$installer" = "yes"; then
		AC_MSG_ERROR([Could not find makensis, which is required for building the installer.])
	fi

	AC_PATH_PROG([ZIP], zip, no)
	AC_ARG_VAR([ZIP], [path to zip archive utility])
	if test "$ZIP" = "no" -a "$installer" = "yes"; then
		AC_MSG_ERROR([Could not find zip, which is required for building the installer.])
	fi

	AM_CONDITIONAL([WITH_INSTALLER], [test "$win32" = "yes" -a "$installer" = "yes" -a "$ZIP" != "no" -a "$MAKENSIS" != "no"])
	AC_CONFIG_FILES([win32/mcus.nsi])
fi

dnl ***************************************************************************
dnl Internationalisation
dnl ***************************************************************************

GETTEXT_PACKAGE=mcus
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])
AC_CONFIG_FILES([po/Makefile.in])

dnl ***************************************************************************
dnl Documentation
dnl ***************************************************************************

GNOME_DOC_INIT([0.17.1], [docs=yes], [docs=no])

if test "$docs" = "yes"; then
	AC_PATH_PROG([XMLLINT], xmllint, no)
	AC_ARG_VAR([XMLLINT], [path to xmllint utility])
	if test "$XMLLINT" = "no"; then
		AC_MSG_ERROR([Could not find xmllint, which is required for building PDF documentation.])
	fi

	AC_PATH_PROG([DB2PDF], docbook2pdf, no)
	AC_ARG_VAR([DB2PDF], [path to docbook2pdf utility])
	if test "$DB2PDF" = "no"; then
		AC_MSG_ERROR([Could not find docbook2pdf, which is required for building PDF documentation.])
	fi
fi

AC_CONFIG_FILES([help/Makefile])

dnl ***************************************************************************
dnl Compilation
dnl ***************************************************************************

AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])

PKG_CHECK_MODULES(STANDARD, glib-2.0 >= 2.16 gtk+-2.0 >= 2.18 gmodule-2.0 gtksourceview-2.0 gthread-2.0)
AC_SUBST(STANDARD_CFLAGS)
AC_SUBST(STANDARD_LIBS)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
