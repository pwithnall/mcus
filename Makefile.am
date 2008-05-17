SUBDIRS = src po data

mcusdocdir = ${prefix}/doc/mcus
mcusdoc_DATA = \
	README		\
	COPYING		\
	AUTHORS		\
	INSTALL		\
	NEWS

EXTRA_DIST =			\
	autogen.sh		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	$(mcusdoc_DATA)

DISTCLEANFILES = \
	intltool-extract	\
	intltool-merge		\
	intltool-update

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

