<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:import href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:param name="timeField" select="timeField"/>
  <xsl:param name="depthField" select="depthField"/>
  <xsl:param name="tempField" select="tempField"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:variable name="lf"><xsl:text>
</xsl:text></xsl:variable>
  <xsl:variable name="fs"><xsl:text>	</xsl:text></xsl:variable>

  <xsl:template match="/">
    <divelog program="subsurface-import" version="2">
      <dives>
        <dive>
          <divecomputerid deviceid="ffffffff" model="stone" />
          <xsl:call-template name="printLine">
            <xsl:with-param name="line" select="substring-before(//csv, $lf)"/>
            <xsl:with-param name="remaining" select="substring-after(//csv, $lf)"/>
          </xsl:call-template>
        </dive>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template name="printLine">
    <xsl:param name="line"/>
    <xsl:param name="remaining"/>
    <xsl:call-template name="printFields">
      <xsl:with-param name="line" select="$line"/>
    </xsl:call-template>
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

    <!-- First field should be dive time. If the value is not numeric,
         we'll skip it. (We do also allow time in h:m:s notation.) -->

    <xsl:if test="number($value) = $value or number(substring-before($value, ':')) = substring-before($value, ':')">
      <sample>
        <xsl:attribute name="time">
          <xsl:choose>
            <xsl:when test="number($value) = $value">
              <!-- We assume time in seconds -->

              <xsl:call-template name="sec2time">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="$value"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <!-- We assume time format h:m:s -->

              <xsl:value-of select="concat(
                substring-before($value, ':') * 60 + substring-before(substring-after($value, ':'), ':'),
                ':',
                substring-after(substring-after($value, ':'), ':')
                )" />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>

        <xsl:attribute name="depth">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$depthField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:attribute>

        <xsl:attribute name="temp">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$tempField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:attribute>
      </sample>
    </xsl:if>
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
        <xsl:value-of select="substring-before($line,$fs)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
