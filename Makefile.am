SUBDIRS = . po help

# MCUS binary
bin_PROGRAMS = src/mcus

src_mcus_SOURCES = \
	src/main.c				\
	src/main.h				\
	src/compiler.c				\
	src/compiler.h				\
	src/instructions.h			\
	src/main-window.c			\
	src/main-window.h			\
	src/simulation.c			\
	src/simulation.h			\
	src/widgets/seven-segment-display.c	\
	src/widgets/seven-segment-display.h	\
	src/widgets/led.c			\
	src/widgets/led.h

src_mcus_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	$(DISABLE_DEPRECATED)						\
	$(AM_CPPFLAGS)

src_mcus_CFLAGS = \
	$(STANDARD_CFLAGS)	\
	$(AM_CFLAGS)

if WIN32
src_mcus_CFLAGS += \
        -mms-bitfields  \
        -mwindows
endif

src_mcus_LDADD = \
	$(STANDARD_LIBS)	\
	$(AM_LDADD)

# Below copied from https://www.redhat.com/archives/libvir-list/2008-October/msg00331.html
if WITH_WIN_ICON
src_mcus_LDADD += src/mcus_win_icon.$(OBJEXT)
src_mcus_DEPENDENCIES = src/mcus_win_icon.$(OBJEXT)

src/mcus_win_icon.$(OBJEXT): data/icons/mcus_win_icon.rc
	$(WINDRES) \
	  --input-format rc --input $< \
	  --output-format coff --output $@
endif

# Icons
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating GTK+ icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

icon16dir = $(datadir)/icons/hicolor/16x16/apps
icon22dir = $(datadir)/icons/hicolor/22x22/apps
icon32dir = $(datadir)/icons/hicolor/32x32/apps
icon48dir = $(datadir)/icons/hicolor/48x48/apps
dist_icon16_DATA = data/icons/16x16/mcus.png
dist_icon22_DATA = data/icons/22x22/mcus.png
dist_icon32_DATA = data/icons/32x32/mcus.png
dist_icon48_DATA = data/icons/48x48/mcus.png

EXTRA_DIST = \
	data/icons/16x16/mcus.svg	\
	data/icons/22x22/mcus.svg	\
	data/icons/32x32/mcus.svg	\
	data/icons/48x48/mcus.svg

# UI file
uidir = $(datadir)/mcus
dist_ui_DATA = \
	data/mcus.ui		\
	data/ocr-assembly.lang

# Desktop file
@INTLTOOL_DESKTOP_RULE@

if WIN32
desktop_in_files =
desktopdir =
desktop_DATA =
else
desktop_in_files = data/mcus.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
endif

EXTRA_DIST += $(desktop_in_files)
CLEANFILES = $(desktop_DATA)

# Example programs
exampledir = $(datadir)/mcus/examples
dist_example_DATA = \
	examples/adc_divider.asm	\
	examples/led_chaser.asm		\
	examples/scrolling_message.asm	\
	examples/ssd_tester.asm

# Cleaning
EXTRA_DIST += \
	ChangeLog.pre-git	\
	autogen.sh

CLEANFILES += \
	`find "$(srcdir)" -type f -name .dirstamp -print`

DISTCLEANFILES = \
	gnome-doc-utils.make

MAINTAINERCLEANFILES = \
	INSTALL		\
	aclocal.m4	\
	compile		\
	config.guess	\
	config.h.in	\
	config.sub	\
	depcomp		\
	install-sh	\
	ltmain.sh	\
	missing		\
	mkinstalldirs	\
	`find "$(srcdir)" -type f -name Makefile.in -print`

# ChangeLog
ChangeLog: $(srcdir)/ChangeLog
$(srcdir)/ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run \
	   git log --stat --after="Sun Nov 16 15:58:17 2008 +0000") | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi
.PHONY: $(srcdir)/ChangeLog

MAINTAINERCLEANFILES += $(srcdir)/ChangeLog

-include $(top_srcdir)/git.mk
