<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="xml" encoding="UTF-8" indent="yes"/>
  <xsl:strip-space elements="*"/>

  <xsl:template match="/">
    <kml xmlns="http://www.opengis.net/kml/2.2">
      <Document>
        <name>Subsurface dive sites</name>
        <xsl:apply-templates select="divelog/divesites/site[string-length(normalize-space(@gps)) &gt; 0]"/>
      </Document>
    </kml>
  </xsl:template>

  <xsl:template match="site">
    <!-- The SSRF serializer places one formatting space before </notes>. -->
    <xsl:variable name="notes" select="substring(notes, 1, string-length(notes) - 1)"/>
    <Placemark xmlns="http://www.opengis.net/kml/2.2">
      <name><xsl:value-of select="@name"/></name>
      <xsl:if test="@description != '' or $notes != ''">
        <description>
          <xsl:value-of select="@description"/>
          <xsl:if test="@description != '' and $notes != ''"><xsl:text>&#10;</xsl:text></xsl:if>
          <xsl:value-of select="$notes"/>
        </description>
      </xsl:if>
      <Point>
        <coordinates>
          <xsl:value-of select="substring-after(normalize-space(@gps), ' ')"/>
          <xsl:text>,</xsl:text>
          <xsl:value-of select="substring-before(normalize-space(@gps), ' ')"/>
          <xsl:text>,0</xsl:text>
        </coordinates>
      </Point>
    </Placemark>
  </xsl:template>
</xsl:stylesheet>
