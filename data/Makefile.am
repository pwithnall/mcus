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
