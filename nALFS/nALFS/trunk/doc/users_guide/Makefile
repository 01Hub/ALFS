OUTDIR=./output/
XSLROOTDIR=/usr/share/xml/docbook/xsl-stylesheets-current/

clean:
	if [ -e $(OUTDIR) ]; then \
	  rm -rf $(OUTDIR); \
	fi;

	if [ -e nALFS-USERS-GUIDE ]; then \
	  rm -rf nALFS-USERS-GUIDE; \
	fi;

dist-clean: clean
	if [ -e *.bz2 ]; then \
	  rm *.bz2; \
	fi;

	if [ -e *.html ]; then \
	  rm *.html; \
	fi;

	if [ -e *.txt ]; then \
	  rm *.txt; \
	fi;

common: clean
	if [ ! -e $(OUTDIR) ]; then \
	  mkdir -p $(OUTDIR); \
	fi;

chunks-html: common
	if [ ! -e $(OUTDIR)stylesheets ]; then \
	  mkdir -p $(OUTDIR)stylesheets; \
	fi;

	if [ ! -e $(OUTDIR)images ]; then \
	  mkdir -p $(OUTDIR)images; \
	fi;

	sed -e "s@XSLROOTDIR@$(XSLROOTDIR)@" \
	  stylesheets/alfs-chunked.xsl \
	  > $(OUTDIR)stylesheets/alfs-chunked.xsl

	xsltproc -nonet -stringparam base.dir $(OUTDIR) \
	  $(OUTDIR)stylesheets/alfs-chunked.xsl \
	  index.xml

	cp stylesheets/alfs.css $(OUTDIR)stylesheets

	cp $(XSLROOTDIR)images/*.png \
	  $(OUTDIR)images

	sed -i -e "s@../stylesheets@stylesheets@" \
	  $(OUTDIR)index.html

	sed -i -e "s@../stylesheets@../../stylesheets@" \
	  $(OUTDIR)introduction/welcome/*.html 

	sed -i -e "s@images@../images@g" \
	  $(OUTDIR)chapter01/*.html $(OUTDIR)introduction/*.html \
	  $(OUTDIR)preface/*.html

	sed -i -e "s@images@../../images@g" \
	  $(OUTDIR)introduction/welcome/*.html

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
	rm $(OUTDIR)stylesheets/alfs-chunked.xsl
	mv $(OUTDIR) nALFS-USERS-GUIDE

	tar -cjvf nALFS-USERS-GUIDE-SVN-CHUNKS-html.tar.bz2 nALFS-USERS-GUIDE/*

nochunks-html: common
	sed -e "s@XSLROOTDIR@$(XSLROOTDIR)@" \
	  stylesheets/alfs-onechunk.xsl \
	  > $(OUTDIR)alfs-onechunk.xsl

	xsltproc --nonet -stringparam base.dir $(OUTDIR) \
	  $(OUTDIR)alfs-onechunk.xsl \
	  index.xml

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
	rm $(OUTDIR)alfs-onechunk.xsl
	mv $(OUTDIR)index.html nALFS-USERS-GUIDE-SVN-NOCHUNKS.html

flat-text: nochunks-html
	lynx -dump nALFS-USERS-GUIDE-SVN-NOCHUNKS.html > nALFS-USERS-GUIDE-SVN.txt

.PHONY: clean dist-clean common chunks-html nochunks-html flat-text
