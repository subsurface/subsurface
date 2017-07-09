<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:param name="units" select="units"/>
  <xsl:output method="text" encoding="UTF-8"/>

  <xsl:variable name="fs">,</xsl:variable>

  <xsl:template match="/divelog/dives">
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat('&quot;dive number&quot;', $fs, '&quot;date&quot;', $fs, '&quot;time&quot;', $fs, '&quot;sample time&quot;', $fs, '&quot;sample depth (ft)&quot;', $fs, '&quot;sample temperature (F)&quot;', $fs, '&quot;sample pressure (psi)&quot;')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&quot;dive number&quot;', $fs, '&quot;date&quot;', $fs, '&quot;time&quot;', $fs, '&quot;sample time&quot;', $fs, '&quot;sample depth (m)&quot;', $fs, '&quot;sample temperature (C)&quot;', $fs, '&quot;sample pressure (bar)&quot;')"/>
      </xsl:otherwise>
    </xsl:choose>
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
    <xsl:for-each select="divecomputer[1]/sample|sample">
      <xsl:value-of select="concat('&quot;', $number, '&quot;')"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat('&quot;', $date, '&quot;')"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat('&quot;', $time, '&quot;')"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat('&quot;', substring-before(@time, ' '), '&quot;')"/>
      <xsl:value-of select="$fs"/>
      <xsl:choose>
        <xsl:when test="$units = 1">
          <xsl:value-of select="concat('&quot;', round((substring-before(@depth, ' ') div 0.3048) * 1000) div 1000, '&quot;')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('&quot;', round(substring-before(@depth, ' ') * 1000) div 1000, '&quot;')"/>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:value-of select="$fs"/>
      <xsl:if test="@temp != ''">
        <xsl:choose>
          <xsl:when test="$units = 1">
            <xsl:value-of select="concat('&quot;', format-number((substring-before(@temp, ' ') * 1.8) + 32, '#.#'), '&quot;')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('&quot;', substring-before(@temp, ' '), '&quot;')"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>

      <xsl:value-of select="$fs"/>
      <xsl:if test="@pressure != ''">
        <xsl:choose>
          <xsl:when test="$units = 1">
            <xsl:value-of select="concat('&quot;', format-number((substring-before(@pressure, ' ') * 14.5037738007), '#'), '&quot;')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('&quot;', substring-before(@pressure, ' '), '&quot;')"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>

      <xsl:text>
</xsl:text>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>
