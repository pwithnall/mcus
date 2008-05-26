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
	parser.c		\
	parser.h		\
	instructions.h		\
	main-window.c		\
	main-window.h		\
	simulation.c		\
	simulation.h

mcus_LDFLAGS = 
mcus_LDADD = $(STANDARD_LIBS)
