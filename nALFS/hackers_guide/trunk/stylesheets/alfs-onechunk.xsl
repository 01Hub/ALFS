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
  <xsl:import href="XSLROOTDIRxhtml/onechunk.xsl"/>

<!-- Our custom CSS file to make everything look pretty. -->
  <xsl:template name='user.head.content'>
    <style type="text/css">
      <xsl:text>
div.navheader table
{
    font-weight: normal;
    font-size: 8.5pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.navfooter table
{
    font-weight: normal;
    font-size: 8.5pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.navheader img
{
    border-right: medium none;
    border-top: medium none;
    border-left: medium none;
    border-bottom: medium none
}
div.navfooter img
{
    border-right: medium none;
    border-top: medium none;
    border-left: medium none;
    border-bottom: medium none
}
div.book {
    font-weight: normal;
    font-size: 10pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.part {
    font-weight: normal;
    font-size: 10pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.chapter {
    font-weight: normal;
    font-size: 10pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.book div.titlepage h3.author {
    font-weight: normal;
    font-size: 14pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.book div.dedication div.titlepage h2.title {
    font-weight: normal;
    font-size: 14pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.book div.titlepage h1.title
{
    font-weight: bold;
    font-size: 16pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: center
}
div.part div.titlepage h1.title
{
    font-weight: bold;
    font-size: 16pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.book div.titlepage h2.subtitle
{
    font-weight: bold;
    font-size: 14pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: center
}
div.legalnotice {
    font-weight: bold;
    font-size: 10pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.preface div.titlepage h2.title
{
    font-weight: bold;
    font-size: 16pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left;
    border-top: 1px solid
}
div.chapter div.titlepage h2.title
{
    font-weight: bold;
    font-size: 16pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left;
    border-top: 1px solid
}
div.sect1 div.titlepage h2.title
{
    font-weight: bold;
    font-size: 14pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
div.sect1
{
    font-weight: normal;
    font-size: 10pt;
    font-family: veranda, tahoma, helvetica, arial, sans-serif;
    text-align: left
}
tt
{
    font-family: courier, monospace;
}
pre.screen
{
    font-family: courier, monospace;
    color: black;
    background-color: #dddddd
}
pre.synopsis
{
    color: black;
    background-color: #dddddd
}
div.warning
{
    border-right: 1px solid;
    border-top: 1px solid;
    border-left: 1px solid;
    border-bottom: 1px solid
}
div.note
{
    border-right: 1px solid;
    border-top: 1px solid;
    border-left: 1px solid;
    border-bottom: 1px solid
}
div.important
{
    border-right: 1px solid;
    border-top: 1px solid;
    border-left: 1px solid;
    border-bottom: 1px solid
}
div.caution
{
    border-right: 1px solid;
    border-top: 1px solid;
    border-left: 1px solid;
    border-bottom: 1px solid
}
div.warning h3.title
{
    text-align: center
}
div.warning p
{
    padding-left: 0.2in
}
div.note
{
    padding-left: 0.2in
}
div.important
{
    padding-left: 0.2in
}
div.caution
{
    padding-left: 0.2in
}
      </xsl:text>
    </style>
  </xsl:template>

<!-- Define the encoding for tidy later -->
  <xsl:param name="chunker.output.encoding" select="'ISO-8859-1'"/>

<!-- Generate a clean legal notice link -->
  <xsl:param name="generate.legalnotice.link" select="0"/>
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
