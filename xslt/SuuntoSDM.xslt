<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes" encoding="ASCII"/>

  <xsl:template match="/">
    <dives>
      <program name="subsurface-import" version="1"/>
      <xsl:apply-templates select="/SUUNTO/MSG"/>
    </dives>
  </xsl:template>

  <xsl:template match="MSG">
    <xsl:variable name="units" select="'si'"/>

    <dive number="{DIVENUMBER}">
      <xsl:attribute name="date">
        <xsl:call-template name="dateConvert">
          <xsl:with-param name="date" select="DATE"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:attribute name="time">
        <xsl:value-of select="TIME"/>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:call-template name="timeConvert">
          <xsl:with-param name="timeSec" select="DIVETIMESEC"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:choose>
        <xsl:when test="MEANDEPTH != ''">
          <depth max="{concat(translate(MAXDEPTH, ',', '.'),' m')}" mean="{concat(translate(MEANDEPTH, ',', '.'), ' m')}"/>
        </xsl:when>
        <xsl:otherwise>
          <depth max="{concat(translate(MAXDEPTH, ',', '.'),' m')}"/>
        </xsl:otherwise>
      </xsl:choose>

      <temperature air="{concat(AIRTEMP, ' C')}" water="{concat(WATERTEMPMAXDEPTH, ' C')}"/>
      <xsl:if test="SURFACETIME != '0'">
        <surfacetime>
          <xsl:call-template name="timeConvert">
            <xsl:with-param name="timeSec" select="SURFACETIME" />
            <xsl:with-param name="units" select="$units" />
          </xsl:call-template>
        </surfacetime>
      </xsl:if>
      <divemaster><xsl:value-of select="DIVEMASTER" /></divemaster>
      <buddy><xsl:value-of select="PARTNER" /></buddy>
      <xsl:choose>
        <xsl:when test="LOCATION != ''">
          <location><xsl:value-of select="normalize-space(concat(LOCATION, ' ' , SITE))" /></location>
        </xsl:when>
        <xsl:otherwise>
          <xsl:if test="SITE != ''">
            <location><xsl:value-of select="SITE" /></location>
          </xsl:if>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:if test="WEIGHT != ''">
        <weightsystem>
          <xsl:attribute name="weight">
            <xsl:value-of select="concat(translate(WEIGHT, ',', '.'), ' kg')"/>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

      <notes>
      <xsl:if test="LOGNOTES != ''">
        <xsl:value-of select="LOGNOTES" />
      </xsl:if>
      <xsl:if test="WEATHER != ''">
        Weather: <xsl:value-of select="WEATHER" />
      </xsl:if>
      <xsl:if test="WATERVISIBILITYDESC != ''">
        Visibility: <xsl:value-of select="WATERVISIBILITYDESC" />
      </xsl:if>
      <xsl:if test="BOATNAME != ''">
        Boat name: <xsl:value-of select="BOATNAME" />
      </xsl:if>
      </notes>

<!-- FIXME: add support for multiple cylinders, need sample data -->
      <cylinder>
      <xsl:attribute name="o2">
        <xsl:choose>
          <xsl:when test="O2PCT != ''">
            <xsl:value-of select="concat(O2PCT, '%')"/>
          </xsl:when>
          <xsl:otherwise>21.0%</xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:if test="HEPCT != ''">
        <xsl:attribute name="he">
          <xsl:value-of select="concat(HEPCT, '%')"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:if test="CYLINDERDESCRIPTION != ''">
        <xsl:attribute name="description">
          <xsl:value-of select="CYLINDERDESCRIPTION"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:if test="CYLINDERSIZE != ''">
        <xsl:attribute name="size">
            <xsl:choose>
              <xsl:when test="CYLINDERUNITS = '0'">
                <xsl:value-of select="concat(CYLINDERSIZE, ' l')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:call-template name="cuft2l">
                  <xsl:with-param name="size" select="CYLINDERSIZE"/>
                  <xsl:with-param name="pressure" select="CYLINDERWORKPRESSURE"/>
                </xsl:call-template>
              </xsl:otherwise>
            </xsl:choose>
        </xsl:attribute>
      </xsl:if>
      <xsl:attribute name="start">
        <xsl:variable name="number" select="CYLINDERSTARTPRESSURE"/>
        <xsl:call-template name="pressure">
          <xsl:with-param name="number" select="$number"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:attribute>
      <xsl:attribute name="end">
        <xsl:variable name="number" select="CYLINDERENDPRESSURE"/>
        <xsl:call-template name="pressure">
          <xsl:with-param name="number" select="$number"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:attribute>
      <xsl:if test="CYLINDERWORKPRESSURE != '0'">
        <xsl:attribute name="workpressure">
          <xsl:variable name="number" select="CYLINDERWORKPRESSURE"/>
          <xsl:call-template name="pressure">
            <xsl:with-param name="number" select="$number"/>
            <xsl:with-param name="units" select="$units"/>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:if>
      </cylinder>

      <!-- DELTA is the sample interval -->
      <xsl:variable name="delta">
        <xsl:choose>
          <xsl:when test="SAMPLEINTERVAL != ''">
            <xsl:value-of select="SAMPLEINTERVAL"/>
          </xsl:when>
          <xsl:otherwise>0</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <!-- end delta -->

      <!-- gas change -->
<!-- FIXME: test this with proper data -->
      <xsl:for-each select="GASCHANGE">
        <xsl:if test="MIXTIME != '0'">
          <event name="gaschange">
            <xsl:attribute name="value">
              <xsl:value-of select="MIXVALUE" />
            </xsl:attribute>
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec" select="MIXTIME"/>
                <xsl:with-param name="units" select="'si'"/>
              </xsl:call-template>
            </xsl:attribute>
          </event>
        </xsl:if>
      </xsl:for-each>
      <!-- end gas change-->

      <!-- dive sample - all the depth and temp readings -->
      <xsl:for-each select="SAMPLE">
        <xsl:choose>
          <xsl:when test="BOOKMARK = ''">
            <sample>
              <xsl:attribute name="time">
                <xsl:call-template name="timeConvert">
                  <xsl:with-param name="timeSec" select="SAMPLETIME"/>
                  <xsl:with-param name="units" select="'si'"/>
                </xsl:call-template>
              </xsl:attribute>
              <xsl:attribute name="depth">
                <xsl:value-of select="concat(translate(DEPTH, ',', '.'), ' m')"/>
              </xsl:attribute>
              <xsl:if test="TEMPERATURE &gt; 0">
                <xsl:attribute name="temp">
                  <xsl:value-of select="TEMPERATURE"/>
                </xsl:attribute>
              </xsl:if>
              <xsl:attribute name="pressure">
                <xsl:call-template name="pressure">
                  <xsl:with-param name="number" select="PRESSURE"/>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
            </sample>
          </xsl:when>
          <xsl:when test="substring-before(BOOKMARK, ':') = 'Heading'">
              <event name="heading">
                <xsl:attribute name="value">
                  <xsl:value-of select="substring-before(substring-after(BOOKMARK, ': '), '°')"/>
                </xsl:attribute>
                <xsl:attribute name="time">
                  <xsl:call-template name="timeConvert">
                    <xsl:with-param name="timeSec" select="SAMPLETIME"/>
                    <xsl:with-param name="units" select="'si'"/>
                  </xsl:call-template>
                </xsl:attribute>
              </event>
          </xsl:when>
          <xsl:otherwise>
            <xsl:if test="BOOKMARK != 'Surface'">
              <event name="{BOOKMARK}">
                <xsl:attribute name="time">
                  <xsl:call-template name="timeConvert">
                    <xsl:with-param name="timeSec" select="SAMPLETIME"/>
                    <xsl:with-param name="units" select="'si'"/>
                  </xsl:call-template>
                </xsl:attribute>
              </event>
            </xsl:if>
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

    <xsl:choose>
      <xsl:when test="$number != ''">
        <xsl:value-of select="concat(format-number(($number div 1000), '#.##'), ' bar')"/>
      </xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end convert pressure -->

  <!-- convert cuft to liters-->
  <xsl:template name="cuft2l">
    <xsl:param name="size"/>
    <xsl:param name="pressure"/>
    <xsl:choose>
      <xsl:when test="$pressure != '0'">
        <xsl:value-of select="concat(format-number((($size*28.3168466) div ($pressure div 1013.25)), '0.000'), ' l')" />
      </xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end cuft2l -->

  <!-- convert time in seconds to minutes:seconds -->
  <xsl:template name="timeConvert">
    <xsl:param name="timeSec"/>
    <xsl:param name="units"/>

    <xsl:choose>
      <xsl:when test="$timeSec = '0'">
        <xsl:value-of select="'0:00 min'" />
      </xsl:when>
      <xsl:when test="$units = 'si'">
        <xsl:value-of select="concat(floor(number($timeSec) div 60), ':', format-number(floor(number($timeSec) mod 60), '00'), ' min')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat(substring-before($timeSec, '.'), ':', format-number(substring-after($timeSec, '.'), '00'), ' min')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end convert time -->

  <!-- convert date from dotted notation to dash notation -->
  <xsl:template name="dateConvert">
    <xsl:param name="date" />
    <xsl:value-of select="concat(substring($date,7,4), '-', substring($date,4,2), '-', substring($date,1,2))" />
  </xsl:template>
</xsl:stylesheet>
