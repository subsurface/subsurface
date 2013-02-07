<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <divelog program='subsurface' version='2'>
      <settings>
        <divecomputer>
          <xsl:apply-templates select="/dives/dive/computer"/>
          <xsl:apply-templates select="/dives/dive/serial"/>
        </divecomputer>
      </settings>
      <dives>
        <xsl:apply-templates select="/dives/dive"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="computer">
    <xsl:attribute name="model">
      <xsl:value-of select="."/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="serial">
    <xsl:attribute name="serial">
      <xsl:value-of select="."/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="dive">
    <xsl:variable name="units" select="/dives/units"/>
    <dive>
      <xsl:attribute name="number">
        <xsl:choose>
          <xsl:when test="divenumber != ''">
            <xsl:value-of select="divenumber"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="diveNumber"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>

      <xsl:attribute name="date">
        <xsl:value-of select="date"/>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:call-template name="timeConvert">
          <xsl:with-param name="timeSec" select="duration"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:choose>
        <xsl:when test="maxdepth != ''">
          <depth max="{concat(maxdepth,' m')}" mean="{concat(avgdepth, ' m')}"/>
        </xsl:when>
        <xsl:otherwise>
          <!-- Note: averageDepth is mis-spelled as in received sample -->
          <depth max="{concat(maxDepth,' m')}" mean="{concat(avergeDepth, ' m')}"/>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:variable name="delta">
        <xsl:value-of select="sampleInterval"/>
      </xsl:variable>

      <location>
        <xsl:value-of select="concat(country, ' ', location, ' ', site)"/>
      </location>

      <xsl:if test="sitelat != ''">
        <gps>
          <xsl:value-of select="concat(sitelat, ' ', sitelon)"/>
        </gps>
      </xsl:if>
      <xsl:if test="siteLat != ''">
        <gps>
          <xsl:value-of select="concat(siteLat, ' ', siteLon)"/>
        </gps>
      </xsl:if>

      <notes>
        <xsl:value-of select="notes"/>
      </notes>

      <divecomputer>
        <xsl:attribute name="model">
          <xsl:value-of select="computer"/>
        </xsl:attribute>
      </divecomputer>

      <xsl:if test="o2percent != ''">
        <cylinder>
          <xsl:attribute name="o2">
            <xsl:value-of select="concat(o2percent, '%')"/>
          </xsl:attribute>
        </cylinder>
      </xsl:if>

      <xsl:for-each select="gases/gas">
        <cylinder>
          <xsl:if test="oxygen != ''">
            <xsl:attribute name="o2">
              <xsl:value-of select="concat(oxygen, '%')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="helium != ''">
            <xsl:attribute name="he">
              <xsl:value-of select="concat(helium, '%')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="pressureStart != ''">
            <xsl:attribute name="start">
              <xsl:call-template name="pressureConvert">
                <xsl:with-param name="number" select="pressureStart"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="pressureEnd != ''">
            <xsl:attribute name="end">
              <xsl:call-template name="pressureConvert">
                <xsl:with-param name="number" select="pressureEnd"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="tankSize != ''">
            <xsl:attribute name="size">
              <xsl:value-of select="concat(tankSize, ' l')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="workingPressure != ''">
            <xsl:attribute name="workpressure">
              <xsl:call-template name="pressureConvert">
                <xsl:with-param name="number" select="workingPressure"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
        </cylinder>
      </xsl:for-each>

      <temperature>
        <xsl:if test="tempAir != ''">
          <xsl:attribute name="air">
            <xsl:value-of select="concat(tempAir, ' C')"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="tempLow != ''">
          <xsl:attribute name="water">
            <xsl:value-of select="concat(tempLow, ' C')"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="tempair != ''">
          <xsl:attribute name="air">
            <xsl:value-of select="concat(tempair, ' C')"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="templow != ''">
          <xsl:attribute name="water">
            <xsl:value-of select="concat(templow, ' C')"/>
          </xsl:attribute>
        </xsl:if>
      </temperature>

      <xsl:if test="diveMaster">
        <divemaster>
          <xsl:value-of select="diveMaster"/>
        </divemaster>
      </xsl:if>
      <buddy>
        <xsl:for-each select="buddies/buddy">
          <xsl:choose>
            <xsl:when test="following-sibling::*[1] != ''">
              <xsl:value-of select="concat(., ', ')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="."/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
      </buddy>

      <xsl:if test="weight != ''">
        <weightsystem>
          <xsl:attribute name="weight">
            <xsl:value-of select="weight"/>
          </xsl:attribute>
          <xsl:attribute name="description">
            <xsl:value-of select="'unknown'"/>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

      <xsl:for-each select="samples/sample">
        <sample>
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="time"/>
              </xsl:with-param>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="depth">
            <xsl:value-of select="concat(depth, ' m')"/>
          </xsl:attribute>
          <xsl:if test="pressure != ''">
            <xsl:attribute name="pressure">
              <xsl:value-of select="concat(pressure, ' bar')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="temperature != ''">
            <xsl:attribute name="temp">
              <xsl:value-of select="concat(temperature, ' C')"/>
            </xsl:attribute>
          </xsl:if>
        </sample>

        <xsl:if test="alarm != ''">
          <event>
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="time"/>
                </xsl:with-param>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:attribute name="name">
              <xsl:choose>
                <xsl:when test="alarm = 'attention'">
                  <xsl:value-of select="'violation'"/>
                </xsl:when>
                <xsl:when test="alarm = 'ascent_rate'">
                  <xsl:value-of select="'ascent'"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="alarm"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </event>
        </xsl:if>
      </xsl:for-each>

    </dive>
  </xsl:template>

  <!-- convert pressure to bars -->
  <xsl:template name="pressureConvert">
    <xsl:param name="number"/>
    <xsl:param name="units"/>

    <xsl:choose>
      <xsl:when test="$units = 'Imperial'">
        <xsl:value-of select="concat(round($number div 14.5037738007), ' bar')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($number, ' bar')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end convert pressure -->

  <!-- convert time in seconds to minutes:seconds -->
  <xsl:template name="timeConvert">
    <xsl:param name="timeSec"/>
    <xsl:param name="units"/>

    <xsl:if test="$timeSec != ''">
      <xsl:choose>
        <xsl:when test="$units = 'Metric'">
          <xsl:value-of select="concat(floor(number($timeSec) div 60), ':',    format-number(floor(number($timeSec) mod 60), '00'), ' min')"/>
        </xsl:when>
      </xsl:choose>
    </xsl:if>
  </xsl:template>
  <!-- end convert time -->

</xsl:stylesheet>
