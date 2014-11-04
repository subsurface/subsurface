<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>
  <xsl:include href="commonTemplates.xsl"/>

  <xsl:template match="/">
    <divelog program='subsurface-import' version='2'>
      <dives>
        <xsl:apply-templates select="/dive/diveLog"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="diveLog">
    <xsl:variable name="units">
      <xsl:choose>
        <xsl:when test="imperialUnits = 'true'">
          <xsl:value-of select="'imperial'"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="'metric'"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <dive>
      <xsl:attribute name="number">
        <xsl:value-of select="number"/>
      </xsl:attribute>

      <xsl:variable name="datetime">
        <xsl:call-template name="convertDate">
          <xsl:with-param name="dateTime">
            <xsl:value-of select="startDate"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:variable>
      <xsl:attribute name="date">
        <xsl:value-of select="substring-before($datetime, ' ')"/>
      </xsl:attribute>
      <xsl:attribute name="time">
        <xsl:value-of select="substring-after($datetime, ' ')"/>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:value-of select="concat(maxTime, ' min')"/>
      </xsl:attribute>

      <depth>
        <xsl:attribute name="max">
          <xsl:choose>
            <xsl:when test="$units = 'imperial'">
              <xsl:value-of select="maxDepth * 0.3048"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="maxDepth"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </depth>

      <surface>
        <xsl:attribute name="pressure">
          <xsl:value-of select="concat(startSurfacePressure div 1000, ' bar')"/>
        </xsl:attribute>
      </surface>

      <divecomputer>
        <xsl:attribute name="model">
          <xsl:value-of select="'Shearwater'"/>
        </xsl:attribute>
        <xsl:attribute name="deviceid">
          <xsl:value-of select="computerSerial"/>
        </xsl:attribute>
      </divecomputer>

      <xsl:for-each select="diveLogRecords/diveLogRecord">
        <sample>
          <xsl:attribute name="time">
            <xsl:call-template name="sec2time">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="currentTime"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="depth">
            <xsl:choose>
              <xsl:when test="$units = 'imperial'">
                <xsl:value-of select="format-number(currentDepth * 0.3048, '0.00')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="currentDepth"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
          <xsl:attribute name="temp">
            <xsl:choose>
              <xsl:when test="$units = 'imperial'">
                <xsl:value-of select="concat(format-number((waterTemp - 32) * 5 div 9, '0.0'), ' C')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="waterTemp"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
          <xsl:attribute name="po2">
            <xsl:choose>
              <xsl:when test="$units = 'imperial'">
                <xsl:value-of select="concat(averagePPO2 div 14.5037738, ' bar')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="concat(averagePPO2, ' bar')"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
        </sample>
      </xsl:for-each>
    </dive>
  </xsl:template>
</xsl:stylesheet>
