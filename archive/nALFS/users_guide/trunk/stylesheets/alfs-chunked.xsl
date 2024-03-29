<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns="http://www.w3.org/1999/xhtml"        
	version="1.0">

<!--
        Last Changed Date: $LastChangedDate$
        Last Changed By:   $LastChangedBy$
        Revision:          $LastChangedRevision$ 
-->

<!-- Need to define the root where the XSL stylesheets are installed. -->
  <xsl:import href="XSLROOTDIRxhtml/chunk.xsl"/>

<!-- Our custom CSS file to make everything look pretty. -->
  <xsl:param name="html.stylesheet" select="'../stylesheets/alfs.css'"/>

<!-- Define the encoding for tidy later -->
  <xsl:param name="chunker.output.encoding" select="'ISO-8859-1'"/>

<!-- add the graphics in for note and important -->
  <xsl:param name="admon.graphics" select="1"/>

<!-- Generate a clean legal notice link -->
  <xsl:param name="generate.legalnotice.link" select="1"/>
  <xsl:template match="legalnotice" mode="titlepage.mode">
    <xsl:variable name="id"><xsl:call-template name="object.id"/>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$generate.legalnotice.link != 0">
        <xsl:variable name="filename">
          <xsl:call-template name="make-relative-filename">
            <xsl:with-param name="base.dir" select="''"/>
            <xsl:with-param name="base.name" select="concat($base.dir,'preface/legalnotice.html')"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="title">
          <xsl:apply-templates select="." mode="title.markup"/>
        </xsl:variable>
        <xsl:element name="a">
          <xsl:attribute name="href">
            <xsl:value-of select="'preface/legalnotice.html'"/>
          </xsl:attribute>
          <xsl:copy-of select="$title"/>
        </xsl:element>
        <xsl:call-template name="write.chunk">
          <xsl:with-param name="filename" select="$filename"/>
          <xsl:with-param name="quiet" select="$chunk.quietly"/>
          <xsl:with-param name="content">
            <html>
              <head>
                <xsl:call-template name="system.head.content"/>
                <xsl:call-template name="head.content"/>
                <xsl:call-template name="user.head.content"/>
              </head>
              <body>
                <xsl:call-template name="body.attributes"/>
                <div class="{local-name(.)}">
                  <xsl:apply-templates mode="titlepage.mode"/>
                </div>
              </body>
            </html>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <div class="{local-name(.)}">
          <xsl:apply-templates mode="titlepage.mode"/>
        </div>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

<!-- TOC -->
  <xsl:param name="generate.toc">
    appendix  toc
    book      toc,title,figure,table,example,equation
    chapter   nop
    part      toc
    preface   nop
    qandadiv  nop
    qandaset  nop  
    reference nop
    sect1     nop
    sect2     nop
    sect3     nop
    sect4     nop
    sect5     nop
    section   nop
    set       nop
  </xsl:param>

  <xsl:param name="toc.section.depth">3</xsl:param>
  <xsl:param name="toc.max.depth">3</xsl:param>

</xsl:stylesheet>
