<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:param name="separatorIndex" select="separatorIndex"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:variable name="lf"><xsl:text>
</xsl:text></xsl:variable>
  <xsl:variable name="fs">
    <xsl:choose>
      <xsl:when test="$separatorIndex = 0"><xsl:text>	</xsl:text></xsl:when>
      <xsl:when test="$separatorIndex = 2"><xsl:text>;</xsl:text></xsl:when>
      <xsl:otherwise><xsl:text>,</xsl:text></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="timeField" select="9"/>
  <xsl:variable name="depthField" select="10"/>
  <xsl:variable name="tempField" select="11"/>

  <xsl:template match="/">
    <divelog program="subsurface-import" version="2">
      <dives>
        <dive>
          <xsl:variable name="line">
            <xsl:value-of select="substring-before(substring-after(//SensusCSV, $lf), $lf)"/>
          </xsl:variable>

          <xsl:variable name="year">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="3"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="month">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="4"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="day">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="5"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="hours">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="6"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="seconds">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="7"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>

          <xsl:attribute name="date">
            <xsl:value-of select="concat($year, '-', $month, '-', $day)"/>
          </xsl:attribute>
          <xsl:attribute name="time">
            <xsl:value-of select="concat($hours, ':', $seconds)"/>
          </xsl:attribute>

          <xsl:attribute name="number">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="0"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:attribute>

          <divecomputerid deviceid="ffffffff" model="SensusCSV" />
          <xsl:call-template name="printLine">
            <xsl:with-param name="line" select="substring-before(//SensusCSV, $lf)"/>
            <xsl:with-param name="remaining" select="substring-after(//SensusCSV, $lf)"/>
          </xsl:call-template>
        </dive>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template name="printLine">
    <xsl:param name="line"/>
    <xsl:param name="remaining"/>

    <!-- We only want to process lines with different time stamps, and
         timeField is not necessarily the first field -->
    <xsl:if test="$line != substring-before($remaining, $lf)">
      <xsl:variable name="curTime">
        <xsl:call-template name="getFieldByIndex">
          <xsl:with-param name="index" select="$timeField"/>
          <xsl:with-param name="line" select="$line"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="prevTime">
        <xsl:call-template name="getFieldByIndex">
          <xsl:with-param name="index" select="$timeField"/>
          <xsl:with-param name="line" select="substring-before($remaining, $lf)"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:if test="$curTime &gt;= 0 and $curTime != $prevTime">
        <xsl:call-template name="printFields">
          <xsl:with-param name="line" select="$line"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:if>
    <xsl:if test="$remaining != ''">
      <xsl:call-template name="printLine">
        <xsl:with-param name="line" select="substring-before($remaining, $lf)"/>
        <xsl:with-param name="remaining" select="substring-after($remaining, $lf)"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="printFields">
    <xsl:param name="line"/>

    <xsl:variable name="value">
      <xsl:call-template name="getFieldByIndex">
        <xsl:with-param name="index" select="$timeField"/>
        <xsl:with-param name="line" select="$line"/>
      </xsl:call-template>
    </xsl:variable>

      <sample>
        <xsl:attribute name="time">

              <xsl:call-template name="sec2time">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="$value"/>
                </xsl:with-param>
              </xsl:call-template>
        </xsl:attribute>

        <xsl:variable name="depth">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$depthField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:attribute name="depth">
          <xsl:value-of select="$depth div 1000"/>
        </xsl:attribute>

          <xsl:variable name="temp">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="$tempField"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="tempDec">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="$tempField + 1"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>

          <xsl:variable name="temperature">
            <xsl:choose>
              <xsl:when test="$tempDec != ''">
                <xsl:value-of select="concat($temp, '.', $tempDec)"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$temp"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:attribute name="temp">
            <xsl:value-of select="concat(format-number($temperature - 273.15, '0.00'), ' C')"/>
          </xsl:attribute>

      </sample>
  </xsl:template>

  <xsl:template name="getFieldByIndex">
    <xsl:param name="index"/>
    <xsl:param name="line"/>
    <xsl:choose>
      <xsl:when test="$index > 0">
        <xsl:call-template name="getFieldByIndex">
          <xsl:with-param name="index" select="$index -1"/>
          <xsl:with-param name="line" select="substring-after($line, $fs)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="substring-before($line,$fs) != ''">
            <xsl:value-of select="substring-before($line,$fs)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$line"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
