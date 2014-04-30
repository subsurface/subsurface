<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="text" encoding="UTF-8"/>

  <xsl:variable name="fs">,</xsl:variable>

  <xsl:template match="/divelog/dives">
    <xsl:value-of select="concat('&quot;dive number&quot;', $fs, '&quot;date&quot;', $fs, '&quot;time&quot;', $fs, '&quot;duration&quot;', $fs, '&quot;depth&quot;', $fs, '&quot;temperature&quot;', $fs, '&quot;pressure&quot;')"/>
    <xsl:text>
</xsl:text>
    <xsl:apply-templates select="dive|trip/dive"/>
  </xsl:template>

  <xsl:template match="dive">
    <xsl:variable name="number">
      <xsl:value-of select="@number"/>
    </xsl:variable>
    <xsl:variable name="date">
      <xsl:value-of select="@date"/>
    </xsl:variable>
    <xsl:variable name="time">
      <xsl:value-of select="@time"/>
    </xsl:variable>
    <xsl:for-each select="divecomputer/sample|sample">
      <xsl:value-of select="concat('&quot;', $number, '&quot;')"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat('&quot;', $date, '&quot;')"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat('&quot;', $time, '&quot;')"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat('&quot;', substring-before(@time, ' '), '&quot;')"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat('&quot;', substring-before(@depth, ' '), '&quot;')"/>

      <xsl:value-of select="$fs"/>
      <xsl:if test="@temp != ''">
        <xsl:value-of select="concat('&quot;', substring-before(@temp, ' '), '&quot;')"/>
      </xsl:if>

      <xsl:value-of select="$fs"/>
      <xsl:if test="@pressure != ''">
        <xsl:value-of select="concat('&quot;', substring-before(@pressure, ' '), '&quot;')"/>
      </xsl:if>

      <xsl:text>
</xsl:text>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>
