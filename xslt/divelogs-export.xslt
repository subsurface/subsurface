<?xml version="1.0" encoding="iso-8859-1"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" encoding="UTF-8" indent="yes"
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
      <xsl:value-of select="substring-before(node()/temperature/@air, ' ')"/>
    </AIRTEMP>
    <WATERTEMPMAXDEPTH>
      <xsl:value-of select="substring-before(node()/temperature/@water, ' ')"/>
    </WATERTEMPMAXDEPTH>
    <xsl:variable name="manual">
      <xsl:choose>
        <xsl:when test="divecomputer/@model = 'manually added dive'">
          <xsl:value-of select="1"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="0"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

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
      <xsl:choose>
        <xsl:when test="$manual = 1">
          <xsl:value-of select="$second - $first"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="60"/>
        </xsl:otherwise>
      </xsl:choose>
    </SAMPLEINTERVAL>
    <xsl:for-each select="divecomputer[1]/sample">
      <xsl:choose>
        <xsl:when test="$manual = 1 and @time != '0:00 min'">
          <xsl:variable name="timesecond">
            <xsl:call-template name="time2sec">
              <xsl:with-param name="time" select="@time"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="timefirst">
            <xsl:call-template name="time2sec">
              <xsl:with-param name="time" select="preceding-sibling::sample[1]/@time"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:call-template name="while">
            <xsl:with-param name="until">
              <xsl:value-of select="($timesecond - $timefirst) div 60 - 1"/>
            </xsl:with-param>
            <xsl:with-param name="count">
              <xsl:value-of select="0"/>
            </xsl:with-param>
            <xsl:with-param name="timefirst">
              <xsl:value-of select="$timefirst"/>
            </xsl:with-param>
            <xsl:with-param name="timesecond">
              <xsl:value-of select="$timesecond"/>
            </xsl:with-param>
            <xsl:with-param name="depthsecond">
              <xsl:call-template name="depth2mm">
                <xsl:with-param name="depth">
                  <xsl:value-of select="./@depth"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:with-param>
            <xsl:with-param name="depthfirst">
              <xsl:call-template name="depth2mm">
                <xsl:with-param name="depth">
                  <xsl:value-of select="preceding-sibling::sample[1]/@depth"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:with-param>
          </xsl:call-template>
          <!--  <name select="{concat(@time, ' - ', preceding-sibling::sample[1]/@time)}"/>-->
        </xsl:when>
        <xsl:otherwise>
          <SAMPLE>
            <DEPTH>
              <xsl:value-of select="substring-before(./@depth, ' ')"/>
            </DEPTH>
          </SAMPLE>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    </DIVELOGSDATA>
  </xsl:template>

  <xsl:template name="while">
    <xsl:param name="timefirst"/>
    <xsl:param name="timesecond"/>
    <xsl:param name="depthfirst"/>
    <xsl:param name="depthsecond"/>
    <xsl:param name="count"/>
    <xsl:param name="until"/>

    <xsl:variable name="curdepth">
      <xsl:value-of select="format-number(((($timefirst + 60) - $timefirst) div ($timesecond - $timefirst) * ($depthsecond - $depthfirst) + $depthfirst), '#.##')"/>
    </xsl:variable>
    <xsl:variable name="curtime">
      <xsl:value-of select="$timefirst + 60"/>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$count &lt; $until">
        <SAMPLE>
          <DEPTH>
            <xsl:value-of select="$curdepth div 1000"/>
          </DEPTH>
        </SAMPLE>
        <xsl:call-template name="while">
          <xsl:with-param name="timefirst" select="$curtime"/>
          <xsl:with-param name="timesecond" select="$timesecond"/>
          <xsl:with-param name="depthfirst" select="$curdepth"/>
          <xsl:with-param name="depthsecond" select="$depthsecond"/>
          <xsl:with-param name="count" select="$count + 1"/>
          <xsl:with-param name="until" select="$until"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="$curdepth &gt; 0">
          <SAMPLE>
            <DEPTH>
              <xsl:value-of select="$curdepth div 1000"/>
            </DEPTH>
          </SAMPLE>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>

  </xsl:template>

</xsl:stylesheet>
