XMLLINT = xmllint 
DB2PDF = docbook2pdf

clean-local: doc-clean
distclean-local: doc-clean
mostlyclean-local: doc-clean
maintainer-clean-local: doc-clean

include $(top_srcdir)/gnome-doc-utils.make
dist-hook: doc-dist-hook

DOC_MODULE = mcus
DOC_ENTITIES = 
DOC_INCLUDES = legal.xml
DOC_FIGURES = figures/mcus_main_window.png

C/mcus-pdf.xml: C/mcus.xml
	rm -rf html
	$(mkinstalldirs) html
	$(mkinstalldirs) C
	$(XMLLINT) --xinclude --postvalid $< -o $@

pdf/mcus-pdf.pdf: C/mcus-pdf.xml
	$(DB2PDF) --output pdf $<

pdf: pdf/mcus-pdf.pdf

doc-clean:
	-rm -f C/mcus-pdf.xml
	-rm -rf html
	-rm -rf pdf

all-local: pdf

.PHONY: doc-clean

-include $(top_srcdir)/git.mk
