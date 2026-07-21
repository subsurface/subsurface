<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="xml" encoding="UTF-8" indent="yes" omit-xml-declaration="yes"/>
  <xsl:strip-space elements="*"/>

  <xsl:template match="/divelog">
    <divesites>
      <xsl:attribute name="program"><xsl:value-of select="@program"/></xsl:attribute>
      <xsl:attribute name="version"><xsl:value-of select="@version"/></xsl:attribute>
      <xsl:copy-of select="divesites/site"/>
    </divesites>
  </xsl:template>
</xsl:stylesheet>
