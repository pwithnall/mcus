SUBDIRS = . po

if WITH_DOCS
SUBDIRS += help
endif

# Enums
MCUS_ENUM_FILES = \
	src/simulation-enums.c	\
	src/simulation-enums.h

src/simulation-enums.h: src/simulation.h Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef __MCUS_SIMULATION_ENUMS_H__\n#define __MCUS_SIMULATION_ENUMS_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define MCUS_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* __MCUS_SIMULATION_ENUMS_H__ */" \
		$(srcdir)/src/simulation.h > $@.tmp \
		&& $(SED) "s/m_cus/mcus/" $@.tmp > $@.tmp2 \
		&& $(SED) "s/MCUS_TYPE_CUS/MCUS_TYPE/" $@.tmp2 > $(srcdir)/src/simulation-enums.h \
		&& rm -f $@.tmp $@.tmp2)

src/simulation-enums.c: src/simulation.h Makefile src/simulation-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"simulation.h\"\n#include \"simulation-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(srcdir)/src/simulation.h > $@.tmp \
		&& $(SED) "s/m_cus/mcus/" $@.tmp > $(srcdir)/src/simulation-enums.c \
		&& rm -f $@.tmp)

CLEANFILES = $(MCUS_ENUM_FILES)

# MCUS binary
bin_PROGRAMS = src/mcus

src_mcus_SOURCES = \
	$(MCUS_ENUM_FILES)			\
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
	src/widgets/led.h			\
	src/widgets/byte-array.c		\
	src/widgets/byte-array.h

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
CLEANFILES += src/mcus_win_icon.$(OBJEXT)

src/mcus_win_icon.$(OBJEXT): data/icons/mcus_win_icon.rc
	$(AM_V_GEN)($(WINDRES) \
	  --input-format rc --input $< \
	  --output-format coff --output $@)
endif

# Example programs
exampledir = $(datadir)/mcus/examples
dist_example_DATA = \
	data/examples/adc_divider.asm	\
	data/examples/led_chaser.asm		\
	data/examples/scrolling_message.asm	\
	data/examples/ssd_tester.asm

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
	data/icons/16x16/mcus.svg data/icons/16x16/mcus.ico	\
	data/icons/22x22/mcus.svg data/icons/22x22/mcus.ico	\
	data/icons/32x32/mcus.svg data/icons/32x32/mcus.ico	\
	data/icons/48x48/mcus.svg data/icons/48x48/mcus.ico

# UI file
uidir = $(datadir)/mcus
dist_ui_DATA = \
	data/mcus.ui		\
	data/ocr-assembly.lang

# Desktop file
@INTLTOOL_DESKTOP_RULE@

if WIN32
zdesktop_in_files =
zdesktopdir =
zdesktop_DATA =
else
zdesktop_in_files = data/mcus.desktop.in
zdesktopdir = $(datadir)/applications
zdesktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
endif

EXTRA_DIST += $(zdesktop_in_files)
CLEANFILES += $(zdesktop_DATA)

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

MAINTAINERCLEANFILES += $(srcdir)/ChangeLog

# NSIS installer
if WITH_INSTALLER
installer: win32/mcus-$(PACKAGE_VERSION).zip win32/mcus-$(PACKAGE_VERSION).exe

# Copy the generated files into our Windows build tree
installer-copy-files:
	@cp -R $(top_srcdir)/win32/GTK2-Runtime $(top_builddir)/win32/mcus-$(PACKAGE_VERSION); \
	cp $(top_builddir)/src/.libs/mcus.exe $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)/lib; \
	cp $(top_srcdir)/data/mcus.ui $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)/share/mcus/; \
	cp $(top_srcdir)/data/ocr-assembly.lang $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)/share/mcus/; \
	cp $(top_srcdir)/help/pdf/mcus-pdf.pdf $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)/share/mcus/help.pdf; \
	cp $(top_srcdir)/data/icons/16x16/mcus.png $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)/share/icons/hicolor/16x16/apps; \
	cp $(top_srcdir)/data/icons/22x22/mcus.png $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)/share/icons/hicolor/22x22/apps; \
	cp $(top_srcdir)/data/icons/32x32/mcus.png $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)/share/icons/hicolor/32x32/apps; \
	cp $(top_srcdir)/data/icons/48x48/mcus.png $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)/share/icons/hicolor/48x48/apps; \
	cp $(top_srcdir)/data/examples/*.asm $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)/share/mcus/examples/

installer-clean:
	@rm -rf $(top_builddir)/win32/mcus-$(PACKAGE_VERSION)

# Zip up the Windows build tree
ZIPP = $(ZIPP_$(V))
ZIPP_ = $(ZIPP_$(AM_DEFAULT_VERBOSITY))
ZIPP_0 = echo "  ZIP   " $@; $(ZIP) -q
ZIPP_1 = $(ZIP)

win32/mcus-$(PACKAGE_VERSION).zip: installer-copy-files
	@rm -f $@; \
	cd win32; \
	$(ZIPP) -r mcus-$(PACKAGE_VERSION).zip mcus-$(PACKAGE_VERSION) $(top_srcdir)/win32/windows.reg $(top_srcdir)/win32/README.txt -x \*~

# Building NSIS installer; do some silent-rules magic
NSIS = $(NSIS_$(V))
NSIS_ = $(NSIS_$(AM_DEFAULT_VERBOSITY))
NSIS_0 = @echo "  NSIS  " $@; $(MAKENSIS) -V2
NSIS_1 = $(MAKENSIS)

win32/mcus-$(PACKAGE_VERSION).exe: installer-copy-files
	$(NSIS) win32/mcus.nsi

clean-local: installer-clean
DISTCLEANFILES += $(builddir)/win32/mcus-$(PACKAGE_VERSION).zip $(builddir)/win32/mcus-$(PACKAGE_VERSION).exe
else
installer:
	@echo "Need to reconfigure with --enable-installer to build the installer."
endif

dist-hook: installer
distcheck-hook: installer
.PHONY: $(srcdir)/ChangeLog installer installer-copy-files installer-clean

-include $(top_srcdir)/git.mk
