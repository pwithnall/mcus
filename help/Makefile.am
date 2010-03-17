include $(top_srcdir)/gnome-doc-utils.make
dist-hook: doc-dist-hook

DOC_ID = mcus
DOC_LINGUAS =
DOC_FIGURES = figures/mcus_main_window.png
DOC_PAGES = \
	adc.page		\
	editing.page		\
	index.page		\
	inputs.page		\
	introduction.page	\
	outputs.page		\
	reference.page		\
	simulating.page		\
	lookup-tables.page

# TODO: This isn't localised yet
xhtml:
	$(AM_V_GEN)($(mkinstalldirs) $(top_builddir)/help/C; \
	cd $(top_srcdir)/help/C/; \
	gnome-doc-tool xhtml -n -o $(abs_top_builddir)/help/C/ $(DOC_PAGES))

CLEANFILES = \
	$(addprefix $(top_builddir)/help/C/,$(DOC_PAGES:.page=.xhtml))	\
	$(top_builddir)/help/C/index.cache

all-local: xhtml

-include $(top_srcdir)/git.mk
