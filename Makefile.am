SUBDIRS = src po data examples help

EXTRA_DIST = \
	autogen.sh		\
	gnome-doc-utils.make	\
	mcus.doap		\
	ChangeLog.pre-git

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

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

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
