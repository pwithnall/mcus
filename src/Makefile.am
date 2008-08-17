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
