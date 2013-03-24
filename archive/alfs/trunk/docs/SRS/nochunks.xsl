<?xml version='1.0' encoding='ISO-8859-1'?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://www.w3.org/1999/xhtml"
                version="1.0">

  <xsl:import href="http://docbook.sourceforge.net/release/xsl/1.67.2/xhtml/docbook.xsl"/>

  <!-- Fix encoding issues with default UTF-8 output of the xhtml stylesheet -->
  <xsl:output method="html" encoding="ISO-8859-1" indent="no" />

  <!-- Point to the CSS file -->
  <xsl:param name="html.stylesheet" select="'srs.css'"/>

  <!-- Control the TOC generation -->
  <xsl:param name="generate.toc" select="'book toc'"/>

  <!-- Chapter and section numbering -->
  <xsl:param name="section.autolabel" select="1"/>
  <xsl:param name="section.label.includes.component.label" select="1"/>
</xsl:stylesheet>
