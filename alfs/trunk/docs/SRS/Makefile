BASEDIR=~/alfs-srs
CHUNK_QUIET=0
PDF_OUTPUT=LFS-BOOK.pdf
HTML_OUTPUT=alfs-srs.html
NOCHUNKS_OUTPUT=LFS-BOOK.html
XSLROOTDIR=/usr/share/xml/docbook/xsl-stylesheets-current

alfs-srs:
	xsltproc --xinclude --nonet -o $(BASEDIR)/$(HTML_OUTPUT) \
	  $(XSLROOTDIR)/xhtml/docbook.xsl index.xml

validate:
	xmllint --noout --nonet --xinclude --postvalid index.xml

