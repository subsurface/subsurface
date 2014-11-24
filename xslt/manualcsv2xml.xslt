<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:param name="separatorIndex" select="separatorIndex"/>
  <xsl:param name="units" select="units"/>
  <xsl:param name="dateField" select="dateField"/>
  <xsl:param name="timeField" select="timeField"/>
  <xsl:param name="date" select="date"/>
  <xsl:param name="time" select="time"/>
  <xsl:param name="numberField" select="numberField"/>
  <xsl:param name="durationField" select="durationField"/>
  <xsl:param name="tagsField" select="tagsField"/>
  <xsl:param name="locationField" select="locationField"/>
  <xsl:param name="gpsField" select="gpsField"/>
  <xsl:param name="maxDepthField" select="maxDepthField"/>
  <xsl:param name="meanDepthField" select="meanDepthField"/>
  <xsl:param name="buddyField" select="buddyField"/>
  <xsl:param name="notesField" select="notesField"/>
  <xsl:param name="weightField" select="weightField"/>
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

  <xsl:template match="/">
    <divelog program="subsurface-import" version="2">
      <dives>
        <xsl:call-template name="printLine">
          <xsl:with-param name="line" select="substring-before(//manualCSV, $lf)"/>
          <xsl:with-param name="remaining" select="substring-after(//manualCSV, $lf)"/>
        </xsl:call-template>
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

    <xsl:variable name="number">
      <xsl:choose>
        <xsl:when test="$numberField >= 0">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$numberField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="'0'"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:if test="$number >= 0">
    <dive>
      <xsl:attribute name="date">
        <xsl:choose>
          <xsl:when test="$dateField >= 0">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="$dateField"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat(substring($date, 1, 4), '-', substring($date, 5, 2), '-', substring($date, 7, 2))"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>

      <xsl:attribute name="time">
        <xsl:choose>
          <xsl:when test="$timeField >= 0">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="$timeField"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat(substring($time, 2, 2), ':', substring($time, 4, 2))"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>

      <xsl:if test="$numberField >= 0">
        <xsl:attribute name="number">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$numberField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:if>

      <xsl:if test="$durationField >= 0">
        <xsl:attribute name="duration">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$durationField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:if>

      <xsl:if test="$tagsField >= 0">
        <xsl:attribute name="tags">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$tagsField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:if>

      <divecomputerid deviceid="ffffffff" model="csv" />

      <xsl:if test="$locationField >= 0">
        <location>
          <xsl:if test="$gpsField >= 0">
            <xsl:attribute name="gps">
              <xsl:call-template name="getFieldByIndex">
                <xsl:with-param name="index" select="$gpsField"/>
                <xsl:with-param name="line" select="$line"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$locationField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </location>
      </xsl:if>

      <xsl:if test="$maxDepthField >= 0 or $meanDepthField >= 0">
        <depth>
          <xsl:if test="$maxDepthField >= 0">
            <xsl:attribute name="max">
              <xsl:variable name="maxDepth">
                <xsl:call-template name="getFieldByIndex">
                  <xsl:with-param name="index" select="$maxDepthField"/>
                  <xsl:with-param name="line" select="$line"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="translate($maxDepth, translate($maxDepth, '1234567890,.', ''), '')"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="translate(translate($maxDepth, translate($maxDepth, '1234567890,.', ''), ''), ',', '.') * 0.3048"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$meanDepthField >= 0">
            <xsl:attribute name="mean">
              <xsl:variable name="meanDepth">
                <xsl:call-template name="getFieldByIndex">
                  <xsl:with-param name="index" select="$meanDepthField"/>
                  <xsl:with-param name="line" select="$line"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="translate($meanDepth, translate($meanDepth, '1234567890,.', ''), '')"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="translate(translate($meanDepth, translate($meanDepth, '1234567890,.', ''), ''), ',', '.')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
        </depth>
      </xsl:if>

      <xsl:if test="$buddyField >= 0">
        <buddy>
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$buddyField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </buddy>
      </xsl:if>

      <xsl:if test="$notesField >= 0">
        <notes>
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$notesField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </notes>
      </xsl:if>

      <xsl:if test="$weightField >= 0">
        <weightsystem description="imported">
          <xsl:attribute name="weight">
            <xsl:variable name="weight">
              <xsl:call-template name="getFieldByIndex">
                <xsl:with-param name="index" select="$weightField"/>
                <xsl:with-param name="line" select="$line"/>
              </xsl:call-template>
            </xsl:variable>
            <xsl:choose>
              <xsl:when test="$units = 0">
                <xsl:value-of select="translate($weight, translate($weight, '1234567890,.', ''), '')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="translate(translate($weight, translate($weight, '1234567890,.', ''), ''), ',', '.') * 0.3048"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

    </dive>
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
        <xsl:choose>
          <xsl:when test="substring-before($line,$fs) != ''">
            <xsl:value-of select="substring-before($line,$fs)"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:if test="substring-after($line, $fs) = ''">
              <xsl:value-of select="$line"/>
            </xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
