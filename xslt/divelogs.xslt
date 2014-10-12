<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="no" encoding="UTF-8" omit-xml-declaration="yes"/>

  <xsl:template match="/">
    <divelog program='subsurface-import' version='2'>
      <dives>
        <xsl:apply-templates select="DIVELOGSDATA"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="*">
    <xsl:variable name="delta">
      <xsl:value-of select="SAMPLEINTERVAL"/>
    </xsl:variable>
    <dive>
      <xsl:attribute name="number">
        <xsl:value-of select="DIVELOGSDIVENUMBER"/>
      </xsl:attribute>
      <xsl:attribute name="date">
        <xsl:value-of select="DATE"/>
      </xsl:attribute>
      <xsl:attribute name="time">
        <xsl:value-of select="TIME"/>
      </xsl:attribute>
      <xsl:if test="DIVETIMESEC != ''">
        <xsl:attribute name="duration">
          <xsl:value-of select="concat(floor(number(DIVETIMESEC) div 60), ':', format-number(floor(number(DIVETIMESEC) mod 60), '00'), ' min')"/>
        </xsl:attribute>
    </xsl:if>

      <depth>
        <xsl:if test="MAXDEPTH != ''">
          <xsl:attribute name="max">
            <xsl:value-of select="concat(MAXDEPTH, ' m')"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="MEANDEPTH != ''">
          <xsl:attribute name="mean">
            <xsl:value-of select="concat(MEANDEPTH, ' m')"/>
          </xsl:attribute>
        </xsl:if>
      </depth>
      <location>
        <xsl:for-each select="LOCATION|SITE">
          <xsl:value-of select="."/>
          <xsl:if test=". != '' and following-sibling::SITE[1] != ''"> / </xsl:if>
        </xsl:for-each>
      </location>

      <!-- WEATHER, WATERVIZIBILITY, BOATNAME -->

      <xsl:if test="LAT != ''">
        <gps>
          <xsl:value-of select="concat(LAT, ' ', LNG)"/>
        </gps>
      </xsl:if>

      <temperature>
        <xsl:if test="AIRTEMP != ''">
          <xsl:attribute name="air">
            <xsl:value-of select="AIRTEMP"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="WATERTEMPMAXDEPTH != ''">
          <xsl:attribute name="water">
            <xsl:value-of select="WATERTEMPMAXDEPTH"/>
          </xsl:attribute>
        </xsl:if>
      </temperature>

      <buddy>
        <xsl:value-of select="PARTNER"/>
      </buddy>

      <cylinder>
        <xsl:attribute name="o2">
          <xsl:value-of select="O2PCT"/>
        </xsl:attribute>
        <xsl:if test="HEPCT != ''">
          <xsl:attribute name="he">
            <xsl:value-of select="HEPCT"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:attribute name="start">
          <xsl:value-of select="CYLINDERSTARTPRESSURE"/>
        </xsl:attribute>
        <xsl:attribute name="end">
          <xsl:value-of select="CYLINDERENDPRESSURE"/>
        </xsl:attribute>
        <xsl:if test="CYLINDERSIZE != ''">
          <xsl:attribute name="size">
	    <xsl:value-of select="format-number(CYLINDERSIZE + CYLINDERSIZE * DBLTANK, '#.##')"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="WORKINGPRESSURE &gt; 0">
          <xsl:attribute name="workpressure">
            <xsl:value-of select="WORKINGPRESSURE"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:attribute name="description">
          <xsl:value-of select="CYLINDERDESCRIPTION"/>
        </xsl:attribute>
      </cylinder>

      <xsl:for-each select="ADDITIONALTANKS/TANK">
        <cylinder>
          <xsl:attribute name="o2">
            <xsl:value-of select="O2PCT"/>
          </xsl:attribute>
          <xsl:if test="HEPCT != ''">
            <xsl:attribute name="he">
              <xsl:value-of select="HEPCT"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:attribute name="start">
            <xsl:value-of select="CYLINDERSTARTPRESSURE"/>
          </xsl:attribute>
          <xsl:attribute name="end">
            <xsl:value-of select="CYLINDERENDPRESSURE"/>
          </xsl:attribute>
          <xsl:if test="CYLINDERSIZE != ''">
            <xsl:attribute name="size">
              <xsl:value-of select="format-number(CYLINDERSIZE + CYLINDERSIZE * DBLTANK, '#.##')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="WORKINGPRESSURE &gt; 0">
            <xsl:attribute name="workpressure">
              <xsl:value-of select="WORKINGPRESSURE"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:attribute name="description">
            <xsl:value-of select="CYLINDERDESCRIPTION"/>
          </xsl:attribute>
        </cylinder>
      </xsl:for-each>

      <xsl:if test="WEIGHT != ''">
        <weightsystem>
          <xsl:attribute name="description">
            <xsl:value-of select="'unknown'"/>
          </xsl:attribute>
          <xsl:attribute name="weight">
            <xsl:value-of select="concat(WEIGHT, ' kg')"/>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

      <notes>
        <xsl:value-of select="LOGNOTES"/>
      </notes>

      <xsl:for-each select="SAMPLE/DEPTH">
        <sample>
          <xsl:variable name="timeSec" select="(position() - 1) * $delta"/>
          <xsl:attribute name="time">
            <xsl:value-of select="concat(floor($timeSec div 60), ':',
              format-number(floor($timeSec mod 60), '00'), ' min')"/>
          </xsl:attribute>
          <xsl:attribute name="depth">
            <xsl:value-of select="concat(., ' m')"/>
          </xsl:attribute>
        </sample>
      </xsl:for-each>
    </dive>
  </xsl:template>
</xsl:stylesheet>
