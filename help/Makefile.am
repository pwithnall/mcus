#clean-local: doc-clean
#distclean-local: doc-clean
#mostlyclean-local: doc-clean
#maintainer-clean-local: doc-clean

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
	simulating.page

#C/mcus-pdf.xml: C/mcus.xml
#	$(AM_V_GEN)(rm -rf html
#	$(mkinstalldirs) html
#	$(mkinstalldirs) C
#	$(XMLLINT) --xinclude --postvalid $< -o $@.tmp
#	sed "s\\figures/\\../$(srcdir)/C/figures/\\" $@.tmp > $@
#	rm -f $@.tmp)

#pdf/mcus-pdf.pdf: C/mcus-pdf.xml
#	$(AM_V_GEN)($(DB2PDF) --output pdf $<)

#pdf: pdf/mcus-pdf.pdf

#doc-clean:
#	-rm -f C/mcus-pdf.xml
#	-rm -rf html
#	-rm -rf pdf

#all-local: pdf

#.PHONY: doc-clean

-include $(top_srcdir)/git.mk
