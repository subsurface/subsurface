<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- Convert ISO 8601 time format to "standard" date and time format
       -->
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

  <!-- Convert units in Pascal given in scientific notation to normal
       decimal notation -->
  <xsl:template name="convertPascal">
    <xsl:param name="value"/>

    <xsl:variable name="number">
      <xsl:value-of select="translate($value, 'e', 'E')"/>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="contains($number, 'E')">
        <xsl:variable name="pressure">
          <xsl:choose>
            <xsl:when test="$value != ''">
              <xsl:variable name="Exp" select="substring-after($number, 'E')"/>
              <xsl:variable name="Man" select="substring-before($number, 'E')"/>
              <xsl:variable name="Fac" select="substring('100000000000000000000', 1, substring($Exp,2) + 1)"/>
              <xsl:choose>
                <xsl:when test="$Exp != ''">
                  <xsl:value-of select="(number($Man) * number($Fac))"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="$number"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:when>
            <xsl:otherwise>0</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <xsl:value-of select="$pressure div 100000"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$value"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="time2sec">
    <xsl:param name="time"/>

    <xsl:value-of select="substring-before($time, ':') * 60 + substring-before(substring-after($time, ':'), ' ')"/>
  </xsl:template>

  <!-- Calculate sum of all parameters, and strip any unit following the
       value -->
  <xsl:template name="sum">
    <xsl:param name="values"/>
    <xsl:param name="sum" select="'0'"/>

    <xsl:variable name="value" select="substring-before($values[1], ' ')"/>
    <xsl:choose>
      <xsl:when test="count($values) = 1">
        <xsl:value-of select="format-number($value + $sum, '#.#')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="sum">
          <xsl:with-param name="values" select="$values[position() &gt; 1]"/>
          <xsl:with-param name="sum" select="$sum + $value"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
