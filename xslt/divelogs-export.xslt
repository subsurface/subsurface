<?xml version="1.0" encoding="iso-8859-1"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:import href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" encoding="iso-8859-1" indent="yes"
    cdata-section-elements="LOCATION SITE WEATHER WATERVIZIBILITY PARTNER BOATNAME CYLINDERDESCRIPTION LOGNOTES"
    />

  <xsl:template match="/divelog/dives">
      <xsl:apply-templates select="dive"/>
  </xsl:template>

  <xsl:template match="dive">
    <DIVELOGSDATA>
    <DIVELOGSNUMBER>
      <xsl:value-of select="@number"/>
    </DIVELOGSNUMBER>
    <DATE>
      <xsl:value-of select="concat(substring-after(substring-after(@date, '-'), '-'), '.', substring-before(substring-after(@date, '-'), '-'), '.', substring-before(@date, '-'))"/>
    </DATE>
    <TIME>
      <xsl:value-of select="@time"/>
    </TIME>
    <DIVETIMESEC>
      <xsl:call-template name="time2sec">
        <xsl:with-param name="time">
          <xsl:value-of select="@duration"/>
        </xsl:with-param>
      </xsl:call-template>
    </DIVETIMESEC>
    <LOCATION>
      <xsl:value-of select="location"/>
    </LOCATION>
    <WATERVIZIBILITY>
      <xsl:value-of select="@visibility"/>
    </WATERVIZIBILITY>
    <PARTNER>
      <xsl:value-of select="buddy"/>
    </PARTNER>
    <CYLINDERDESCRIPTION>
      <xsl:value-of select="cylinder/@description"/>
    </CYLINDERDESCRIPTION>
    <CYLINDERSIZE>
      <xsl:value-of select="substring-before(cylinder/@size, ' ')"/>
    </CYLINDERSIZE>
    <CYLINDERSTARTPRESSURE>
      <xsl:choose>
        <xsl:when test="node()/sample/@pressure != ''">
          <xsl:value-of select="substring-before(node()/sample/@pressure, ' ')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="cylinder[1]/@start"/>
        </xsl:otherwise>
      </xsl:choose>
    </CYLINDERSTARTPRESSURE>
    <CYLINDERENDPRESSURE>
      <xsl:choose>
        <xsl:when test="count(node()/sample[@pressure!='']) &gt; 0">
          <xsl:value-of select="node()/sample[@pressure][last()]/@pressure"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="cylinder[1]/@end"/>
        </xsl:otherwise>
      </xsl:choose>
    </CYLINDERENDPRESSURE>
    <WEIGHT>
      <xsl:call-template name="sum">
        <xsl:with-param name="values" select="weightsystem/@weight"/>
      </xsl:call-template>
    </WEIGHT>
    <O2PCT>
      <xsl:value-of select="substring-before(cylinder/@o2, '%')"/>
    </O2PCT>
    <LOGNOTES>
      <xsl:value-of select="notes"/>
    </LOGNOTES>
    <LAT>
      <xsl:value-of select="substring-before(location/@gps, ' ')"/>
    </LAT>
    <LNG>
      <xsl:value-of select="substring-after(location/@gps, ' ')"/>
    </LNG>
    <MAXDEPTH>
      <xsl:value-of select="substring-before(node()/depth/@max, ' ')"/>
    </MAXDEPTH>
    <MEANDEPTH>
      <xsl:value-of select="substring-before(node()/depth/@mean, ' ')"/>
    </MEANDEPTH>
    <AIRTEMP>
      <xsl:value-of select="substring-before(divetemperature/@air, ' ')"/>
    </AIRTEMP>
    <WATERTEMPMAXDEPTH>
      <xsl:value-of select="substring-before(node()/temperature/@water, ' ')"/>
    </WATERTEMPMAXDEPTH>
    <SAMPLEINTERVAL>
      <xsl:variable name="first">
        <xsl:call-template name="time2sec">
          <xsl:with-param name="time">
            <xsl:value-of select="node()/sample[1]/@time"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="second">
        <xsl:call-template name="time2sec">
          <xsl:with-param name="time">
            <xsl:value-of select="node()/sample[2]/@time"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="$second - $first"/>
    </SAMPLEINTERVAL>
    <xsl:for-each select="divecomputer[1]/sample">
      <SAMPLE>
        <DEPTH>
          <xsl:value-of select="substring-before(./@depth, ' ')"/>
        </DEPTH>
      </SAMPLE>
    </xsl:for-each>
    </DIVELOGSDATA>
  </xsl:template>

</xsl:stylesheet>
