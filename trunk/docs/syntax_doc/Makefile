OUTDIR=./output/render/
XSLROOTDIR=/usr/share/docbook-xsl/

clean:
	if [ -e $(OUTDIR) ]; then \
	  rm -rf $(OUTDIR); \
	fi;

chunks-html:
	if [ ! -e $(OUTDIR) ]; then \
	  mkdir -p $(OUTDIR); \
	fi;

	if [ ! -e $(OUTDIR)stylesheets ]; then \
	  mkdir -p $(OUTDIR)stylesheets; \
	fi;

	if [ ! -e $(OUTDIR)../stylesheets ]; then \
	  mkdir -p $(OUTDIR)../stylesheets; \
	fi;

	if [ ! -e $(OUTDIR)images ]; then \
	  mkdir -p $(OUTDIR)images; \
	fi;

	cp index.xml $(OUTDIR)..
	cp -R --parents alfs_dtd/* $(OUTDIR)..
	cp -R --parents book/* $(OUTDIR)..
	cp -R --parents preface/* $(OUTDIR)..

	bash stylesheets/entResolver $(OUTDIR)

	sed -e "s@XSLROOTDIR@$(XSLROOTDIR)@" \
	  stylesheets/alfs-chunked.xsl \
	  > $(OUTDIR)../stylesheets/alfs-chunked.xsl

	xsltproc --nonet -stringparam base.dir $(OUTDIR) \
	  $(OUTDIR)../stylesheets/alfs-chunked.xsl \
	  $(OUTDIR)../index.xml

	cp stylesheets/alfs.css $(OUTDIR)stylesheets

	cp $(XSLROOTDIR)images/*.png \
	  $(OUTDIR)images

	sed -i -e "s@../stylesheets@stylesheets@" \
	  $(OUTDIR)alfs_dtd.html $(OUTDIR)index.html $(OUTDIR)preface.html

	sed -i -e "s@../images@images@g" \
	  $(OUTDIR)alfs_dtd.html $(OUTDIR)index.html $(OUTDIR)preface.html

	echo indent-spaces: 2 > $(OUTDIR)tidy.conf
	echo wrap: 78 >> $(OUTDIR)tidy.conf
	echo tab-size: 8 >> $(OUTDIR)tidy.conf
	echo input-encoding: latin1 >> $(OUTDIR)tidy.conf
	echo output-encoding: latin1 >> $(OUTDIR)tidy.conf
	echo newline: CRLF >> $(OUTDIR)tidy.conf
	echo write-back: yes >> $(OUTDIR)tidy.conf
	echo markup: yes >> $(OUTDIR)tidy.conf
	echo show-warnings: no >> $(OUTDIR)tidy.conf
	echo quiet: yes >> $(OUTDIR)tidy.conf
	echo indent: yes >> $(OUTDIR)tidy.conf
	echo hide-endtags: no >> $(OUTDIR)tidy.conf
	echo uppercase-tags: yes >> $(OUTDIR)tidy.conf
	echo logical-emphasis: yes >> $(OUTDIR)tidy.conf
	echo drop-font-tags: yes >> $(OUTDIR)tidy.conf
	echo tidy-mark: no >> $(OUTDIR)tidy.conf
	echo numeric-entities: yes >> $(OUTDIR)tidy.conf

	bash stylesheets/goTidy $(OUTDIR)
	rm $(OUTDIR)tidy.conf


