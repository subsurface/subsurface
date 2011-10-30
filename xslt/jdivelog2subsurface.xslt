<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <dives>
      <program name="subsurface" version="1"/>
      <xsl:apply-templates select="/JDiveLog/JDive"/>
    </dives>
  </xsl:template>

  <xsl:template match="JDive">
    <xsl:variable name="units" select="UNITS"/>

    <dive number="{DiveNum}">
      <xsl:attribute name="date">
        <xsl:value-of select="concat(DATE/YEAR,'-',format-number(DATE/MONTH,
           '00'), '-', format-number(DATE/DAY, '00'))"/>
      </xsl:attribute>

      <xsl:attribute name="time">
        <xsl:value-of select="concat(format-number(TIME/HOUR, '00'), ':',
           format-number(TIME/MINUTE, '00'))"/>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:call-template name="timeConvert">
          <xsl:with-param name="timeSec" select="Duration"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:choose>
        <xsl:when test="Average_Depth != ''">
          <depth max="{concat(Depth,' m')}" mean="{concat(Average_Depth, ' m')}"/>
        </xsl:when>
        <xsl:otherwise>
          <depth max="{concat(Depth,' m')}"/>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:if test="TEMPERATURE != ''">
        <xsl:choose>
          <xsl:when test="$units = 'si'">
            <temperature water="{concat(format-number(TEMPERATURE - 273.15, '00.0'), ' C')}"/>
          </xsl:when>
          <xsl:otherwise>
            <temperature water="{concat(TEMPERATURE, ' C')}"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>

      <xsl:if test="diveSiteId != ''">
        <xsl:variable name="diveSite" select="diveSiteId"/>
        <location>
          <xsl:value-of select="concat(/JDiveLog/Masterdata/DiveSites/DiveSite[privateId=$diveSite]/country, ' ', /JDiveLog/Masterdata/DiveSites/DiveSite[privateId=$diveSite]/state, ' ', /JDiveLog/Masterdata/DiveSites/DiveSite[privateId=$diveSite]/city, ' ', /JDiveLog/Masterdata/DiveSites/DiveSite[privateId=$diveSite]/spot)"/>
        </location>
      </xsl:if>

      <xsl:if test="Buddy">
        <buddy>
          <xsl:value-of select="Buddy"/>
        </buddy>
      </xsl:if>

      <notes>
        <xsl:if test="DiveActivity != ''">
          <xsl:value-of select="DiveActivity"/>
        </xsl:if>
        <xsl:if test="Comment != ''">
          <xsl:if test="DiveActivity != ''">
            <xsl:value-of select="': '"/>
          </xsl:if>
          <xsl:value-of select="Comment"/>
        </xsl:if>
        <xsl:if test="Equipment/Visibility != ''">
Visibility: <xsl:value-of select="Equipment/Visibility"/>
        </xsl:if>
        <xsl:if test="Equipment/Suit != ''">
Suit: <xsl:value-of select="Equipment/Suit"/>
        </xsl:if>
        <xsl:if test="Equipment/Gloves != ''">
Gloves: <xsl:value-of select="Equipment/Gloves"/>
        </xsl:if>
        <xsl:if test="Equipment/Weight != ''">
Weight: <xsl:value-of select="Equipment/Weight"/>
        </xsl:if>
      </notes>

      <!-- cylinder -->
      <xsl:variable name="o2">
        <xsl:choose>
          <xsl:when test="DIVE/GASES/MIX/O2 != ''">
            <xsl:value-of select="concat(DIVE/GASES/MIX/O2*100, '%')"/>
          </xsl:when>
          <xsl:otherwise>21.0%</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="size">
        <xsl:choose>
          <xsl:when test="Equipment/Tanks/Tank/MIX/TANK/TANKVOLUME != ''">
            <xsl:value-of select="concat(Equipment/Tanks/Tank/MIX/TANK/TANKVOLUME * 1000, ' l')"/>
          </xsl:when>
          <xsl:otherwise>0 l</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="start">
        <xsl:variable name="number" select="Equipment/Tanks/Tank/MIX/TANK/PSTART"/>
        <xsl:call-template name="pressure">
          <xsl:with-param name="number" select="$number"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="end">
        <xsl:variable name="number" select="Equipment/Tanks/Tank/MIX/TANK/PEND"/>
        <xsl:call-template name="pressure">
          <xsl:with-param name="number" select="$number"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:variable>

      <cylinder o2="{$o2}" size="{$size}" start="{$start}" end="{$end}"/>
      <!-- end cylinder -->

      <!-- DELTA is the sample interval -->
      <xsl:variable name="delta">
        <xsl:choose>
          <xsl:when test="DIVE/SAMPLES/DELTA != ''">
            <xsl:choose>
              <xsl:when test="$units = 'si'">
                <xsl:value-of select="DIVE/SAMPLES/DELTA"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="round(60 * DIVE/SAMPLES/DELTA)"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>
          <xsl:otherwise>0</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <!-- end delta -->

      <!-- events -->
      <xsl:for-each select="DIVE/SAMPLES/ALARM">
        <xsl:if test=". != 'SURFACE'">
          <event type="" name="{.}">
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec" select="count(preceding-sibling::D) * $delta"/>
                <xsl:with-param name="units" select="'si'"/>
              </xsl:call-template>
            </xsl:attribute>
          </event>
        </xsl:if>
      </xsl:for-each>
      <!-- end events -->

      <!-- dive sample - all the depth and temp readings -->
      <xsl:for-each select="DIVE/SAMPLES/D">
        <xsl:variable name="timeSec" select="(position() - 1) * $delta"/>
	<xsl:variable name="time" select="concat(floor($timeSec div 60), ':',
          format-number(floor($timeSec mod 60), '00'), ' min')"/>
        <xsl:choose>
          <xsl:when test="name(following-sibling::*[1]) = 'TEMPERATURE'">
	    <sample time="{$time}" depth="{concat(., ' m')}"
              temp="{following-sibling::TEMPERATURE}"/>
          </xsl:when>
          <xsl:otherwise>
            <sample time="{$time}" depth="{concat(., ' m')}"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <!-- dive sample -->

    </dive>
  </xsl:template>
  <!-- end JDive -->

  <!-- convert pressure to bars -->
  <xsl:template name="pressure">
    <xsl:param name="number"/>
    <xsl:param name="units"/>

    <xsl:variable name="pressure">
      <xsl:choose>
        <xsl:when test="$number != ''">
          <xsl:variable name="Exp" select="substring-after($number, 'E')"/>
          <xsl:variable name="Man" select="substring-before($number, 'E')"/>
          <xsl:variable name="Fac" select="substring('100000000000000000000', 1, $Exp + 1)"/>
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

    <xsl:choose>
      <xsl:when test="$units = 'si'">
        <xsl:value-of select="concat(($pressure div 100000), ' bar')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($pressure, ' bar')"/>
      </xsl:otherwise>
    </xsl:choose>

  </xsl:template>
  <!-- end convert pressure -->

  <!-- convert time in seconds to minutes:seconds -->
  <xsl:template name="timeConvert">
    <xsl:param name="timeSec"/>
    <xsl:param name="units"/>

    <xsl:choose>
      <xsl:when test="$units = 'si'">
        <xsl:value-of select="concat(floor(number($timeSec) div 60), ':',    format-number(floor(number($timeSec) mod 60), '00'), ' min')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat(substring-before($timeSec, '.'), ':',           format-number(substring-after($timeSec, '.'), '00'), ' min')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end convert time -->

</xsl:stylesheet>
