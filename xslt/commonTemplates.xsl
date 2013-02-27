<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template name="datetime">
    <xsl:param name="value"/>

    <xsl:variable name="date">
      <xsl:choose>
        <xsl:when test="contains($value, 'T')">
          <xsl:value-of select="substring-before($value, 'T')"/>
        </xsl:when>
        <xsl:when test="contains($value, ' ')">
          <xsl:value-of select="substring-before($value, ' ')"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="time">
      <xsl:choose>
        <xsl:when test="contains($value, 'T')">
          <xsl:value-of select="substring-after($value, 'T')"/>
        </xsl:when>
        <xsl:when test="contains($value, ' ')">
          <xsl:value-of select="substring-after($value, ' ')"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:if test="$date != ''">
      <xsl:attribute name="date">
        <xsl:choose>
          <xsl:when test="contains($date, '-')">
            <xsl:value-of select="$date"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:variable name="year" select="substring($date, 1, 4)"/>
            <xsl:variable name="month" select="substring($date, 5, 2)"/>
            <xsl:variable name="day" select="substring($date, 7,2)"/>
            <xsl:value-of select="concat($year,'-',$month,'-',$day)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="$time != ''">
      <xsl:attribute name="time">
        <xsl:choose>
          <xsl:when test="contains($time, ':')">
            <xsl:value-of select="$time"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:variable name="hour" select="substring($time, 1, 2)"/>
            <xsl:variable name="minute" select="substring($time, 3, 2)"/>
            <xsl:variable name="second" select="substring($time, 5,2)"/>
            <xsl:value-of select="concat($hour,':',$minute,':',$second)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
