<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:param name="units" select="units"/>
  <xsl:output method="text" encoding="utf-8"/>

  <xsl:variable name="fs" select="','"/>
  <xsl:variable name="quote" select="'&quot;'"/>

  <xsl:template match="/divelog/dives">
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat($quote, 'dive number', $quote, $fs, $quote, 'date', $quote, $fs, $quote, 'time', $quote, $fs, $quote, 'sample time (min)', $quote, $fs, $quote, 'sample depth (ft)', $quote, $fs, $quote, 'sample temperature (F)', $quote, $fs, $quote, 'sample pressure (psi)', $quote, $fs, $quote, 'sample heartrate', $quote)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($quote, 'dive number', $quote, $fs, $quote, 'date', $quote, $fs, $quote, 'time', $quote, $fs, $quote, 'sample time (min)', $quote, $fs, $quote, 'sample depth (m)', $quote, $fs, $quote, 'sample temperature (C)', $quote, $fs, $quote, 'sample pressure (bar)', $quote, $fs, $quote, 'sample heartrate', $quote)"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>
</xsl:text>
    <xsl:apply-templates select="dive|trip/dive"/>
  </xsl:template>

  <xsl:template match="divesites/site/notes"/>

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
      <xsl:value-of select="concat($quote, $number, $quote)"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat($quote, $date, $quote)"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat($quote, $time, $quote)"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="concat($quote, substring-before(@time, ' '), $quote)"/>
      <xsl:value-of select="$fs"/>
      <xsl:choose>
        <xsl:when test="$units = 1">
          <xsl:value-of select="concat($quote, format-number(substring-before(@depth, ' ') div 0.3048, '#.###'), $quote)"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat($quote, format-number(substring-before(@depth, ' '), '#.###'), $quote)"/>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:value-of select="$fs"/>
      <xsl:if test="@temp != ''">
        <xsl:choose>
          <xsl:when test="$units = 1">
            <xsl:value-of select="concat($quote, format-number((substring-before(@temp, ' ') * 1.8) + 32, '#.#'), $quote)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($quote, substring-before(@temp, ' '), $quote)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>

      <xsl:value-of select="$fs"/>
      <xsl:if test="@pressure != ''">
        <xsl:choose>
          <xsl:when test="$units = 1">
            <xsl:value-of select="concat($quote, format-number((substring-before(@pressure, ' ') * 14.5037738007), '#'), $quote)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($quote, substring-before(@pressure, ' '), $quote)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>

      <xsl:value-of select="$fs"/>
      <xsl:if test="@heartbeat != ''">
        <xsl:value-of select="concat($quote, @heartbeat, $quote)"/>
      </xsl:if>

      <xsl:text>
</xsl:text>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>
