BASEDIR=./render/
XSLROOTDIR=/usr/share/docbook-xsl/

clean:
	if [ -e $(BASEDIR) ]; then \
	  rm -rf $(BASEDIR); \
	fi;

chunks-html:
	if [ ! -e $(BASEDIR) ]; then \
	  mkdir -p $(BASEDIR); \
	fi;

	if [ ! -e $(BASEDIR)/stylesheets ]; then \
	  mkdir -p $(BASEDIR)/stylesheets; \
	fi;

	if [ ! -e $(BASEDIR)/images ]; then \
	  mkdir -p $(BASEDIR)/images; \
	fi;

	sed -e "s@XSLROOTDIR@$(XSLROOTDIR)@" \
	  stylesheets/alfs-chunked.xsl \
	  > $(BASEDIR)/stylesheets/alfs-chunked.xsl

	xsltproc --nonet -stringparam base.dir $(BASEDIR) \
	  $(BASEDIR)/stylesheets/alfs-chunked.xsl index.xml

	cp stylesheets/alfs.css $(BASEDIR)/stylesheets

	cp $(XSLROOTDIR)/images/*.png \
	  $(BASEDIR)/images

	sed -i -e "s@../stylesheets@stylesheets@" \
	  $(BASEDIR)/alfs_dtd.html $(BASEDIR)/index.html $(BASEDIR)/preface.html

	sed -i -e "s@../images@images@g" \
	  $(BASEDIR)/alfs_dtd.html $(BASEDIR)/index.html $(BASEDIR)/preface.html

