SUBDIRS = icons

###############################################################################
# Icons
###############################################################################

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

###############################################################################
# UI file
###############################################################################

uidir = $(datadir)/mcus
ui_DATA = \
	mcus.ui			\
	ocr-assembly.lang

###############################################################################
# Desktop file
###############################################################################

@INTLTOOL_DESKTOP_RULE@

if WIN32
desktop_in_files = 
desktopdir = 
desktop_DATA = 
else
desktop_in_files = mcus.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
endif

###############################################################################
# General
###############################################################################

EXTRA_DIST = \
	$(ui_DATA)		\
	$(desktop_in_files)
CLEANFILES = $(desktop_DATA)

-include $(top_srcdir)/git.mk
