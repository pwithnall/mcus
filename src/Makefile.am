INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\"				\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"				\
	$(STANDARD_CFLAGS)

AM_CFLAGS = \
	-Wall		\
	-g

if DEBUG
AM_CFLAGS += \
	-fprofile-arcs	\
	-ftest-coverage
endif

if WIN32
AM_CFLAGS += \
        -mms-bitfields  \
        -mwindows
endif

bin_PROGRAMS = mcus

mcus_SOURCES = \
	main.c			\
	main.h			\
	interface.c		\
	interface.h		\
	compiler.c		\
	compiler.h		\
	instructions.h		\
	main-window.c		\
	main-window.h		\
	simulation.c		\
	simulation.h		\
	analogue-input.c	\
	analogue-input.h	\
	input-port.c		\
	input-port.h		\
	widgets/seven-segment-display.c	\
	widgets/seven-segment-display.h	\
	widgets/led.c		\
	widgets/led.h

mcus_LDFLAGS = 
mcus_LDADD = $(STANDARD_LIBS)

# Below copied from https://www.redhat.com/archives/libvir-list/2008-October/msg00331.html
if WITH_WIN_ICON
mcus_LDADD += mcus_win_icon.$(OBJEXT)
mcus_DEPENDENCIES = mcus_win_icon.$(OBJEXT)

# Before you edit mcus_win_icon.rc, please note the following
# limitations of the resource file format:
#
# (1) '..' is not permitted in the icon filename field.
# (2) '-' is not permitted in the icon filename field.
# (3) Comments are not permitted in the file.
#
# Windows appears to choose the first <= 32x32 icon it finds
# in the resource file.  Therefore you should list the available
# icons from largest to smallest, and make sure that the 32x32
# icon is the most legible.
#
# Windows .ICO is a special MS-only format.  GIMP and other
# tools can write it.  However there are several variations,
# and Windows seems to do its own colour quantization.  More
# information is needed in this area.

mcus_win_icon.$(OBJEXT): mcus_win_icon.rc
	$(WINDRES) \
	  --input-format rc --input $< \
	  --output-format coff --output $@
endif
