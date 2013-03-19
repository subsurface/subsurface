<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:u="http://www.streit.cc/uddf/3.2/"
  xmlns:u1="http://www.streit.cc/uddf/3.1/"
  exclude-result-prefixes="u"
  version="1.0">
  <xsl:import href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <divelog program="subsurface-import" version="2">
      <settings>
        <divecomputerid deviceid="ffffffff">
          <xsl:apply-templates select="/uddf/generator|/u:uddf/u:generator|/u1:uddf/u1:generator"/>
        </divecomputerid>
      </settings>
      <dives>
        <xsl:apply-templates select="/uddf/profiledata/repetitiongroup/dive|/u:uddf/u:profiledata/u:repetitiongroup/u:dive|/u1:uddf/u1:profiledata/u1:repetitiongroup/u1:dive"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="generator|u:generator|u1:generator">
    <xsl:if test="manufacturer/name|u:manufacturer/u:name|u1:manufacturer/u1:name != ''">
      <xsl:attribute name="model">
        <xsl:value-of select="name|u:name|u1:name"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="name|u:name|u1:name != ''">
      <xsl:attribute name="firmware">
        <xsl:value-of select="manufacturer/name|/u:manufacturer/u:name|/u1:manufacturer/u1:name"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="version|u:version|u1:version != ''">
      <xsl:attribute name="serial">
        <xsl:value-of select="version|u:version|u1:version"/>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

  <xsl:template match="gasdefinitions|u:gasdefinitions|u1:gasdefinitions">
    <xsl:for-each select="mix|u:mix|u1:mix">
      <cylinder>
        <xsl:attribute name="description">
          <xsl:value-of select="name|u:name|u1:name"/>
        </xsl:attribute>

        <xsl:attribute name="o2">
          <xsl:call-template name="gasConvert">
            <xsl:with-param name="mix">
              <xsl:value-of select="o2|u:o2|u1:o2"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:attribute>

        <xsl:attribute name="he">
          <xsl:call-template name="gasConvert">
            <xsl:with-param name="mix">
              <xsl:value-of select="he|u:he|u1:he"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:attribute>
      </cylinder>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="dive|u:dive|u1:dive">
    <dive>
      <!-- Count the amount of temeprature samples during the dive -->
      <xsl:variable name="temperatureSamples">
        <xsl:call-template name="temperatureSamples" select="samples/waypoint/temperature|u:samples/u:waypoint/u:temperature|u1:samples/u1:waypoint/u1:temperature">
          <xsl:with-param name="units" select="'Kelvin'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:choose>
        <xsl:when test="date != ''">
          <xsl:attribute name="date">
            <xsl:value-of select="concat(date/year,'-',format-number(date/month, '00'), '-', format-number(date/day, '00'))"/>
          </xsl:attribute>
          <xsl:attribute name="time">
            <xsl:value-of select="concat(format-number(time/hour, '00'), ':', format-number(time/minute, '00'))"/>
          </xsl:attribute>
        </xsl:when>
        <xsl:when test="informationbeforedive/datetime|u:informationbeforedive/u:datetime|u1:informationbeforedive/u1:datetime != ''">
          <xsl:call-template name="datetime">
            <xsl:with-param name="value">
              <xsl:value-of select="informationbeforedive/datetime|u:informationbeforedive/u:datetime|u1:informationbeforedive/u1:datetime"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>

      <xsl:for-each select="lowesttemperature|informationafterdive/lowesttemperature|u:lowesttemperature|u:informationafterdive/u:lowesttemperature|u1:lowesttemperature|u1:informationafterdive/u1:lowesttemperature">
        <temperature>
          <xsl:if test="$temperatureSamples &gt; 0 or . != 273.15">
            <xsl:attribute name="water">
              <xsl:value-of select="concat(format-number(.- 273.15, '0.0'), ' C')"/>
            </xsl:attribute>
          </xsl:if>
        </temperature>
      </xsl:for-each>

      <divecomputer deviceid="ffffffff">
        <xsl:attribute name="model">
          <xsl:value-of select="/uddf/generator/name|/u:uddf/u:generator/u:name|/u1:uddf/u1:generator/u1:name"/>
        </xsl:attribute>
      </divecomputer>

      <xsl:apply-templates select="/uddf/gasdefinitions|/u:uddf/u:gasdefinitions|/u1:uddf/u1:gasdefinitions"/>
      <depth>
        <xsl:for-each select="greatestdepth|informationafterdive/greatestdepth|u:greatestdepth|u:informationafterdive/u:greatestdepth|u1:greatestdepth|u1:informationafterdive/u1:greatestdepth">
          <xsl:attribute name="max">
            <xsl:value-of select="concat(., ' m')"/>
          </xsl:attribute>
        </xsl:for-each>
        <xsl:for-each select="averagedepth|informationafterdive/averagedepth|u:averagedepth|u:informationafterdive/u:averagedepth|u1:averagedepth|u1:informationafterdive/u1:averagedepth">
          <xsl:attribute name="mean">
            <xsl:value-of select="concat(., ' m')"/>
          </xsl:attribute>
        </xsl:for-each>
      </depth>

      <xsl:for-each select="samples/waypoint/switchmix|u:samples/u:waypoint/u:switchmix|u1:samples/u1:waypoint/u1:switchmix">
			<!-- Index to lookup gas per cent -->
        <xsl:variable name="idx">
          <xsl:value-of select="./@ref"/>
        </xsl:variable>

        <event name="gaschange" type="11">
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="preceding-sibling::divetime|preceding-sibling::u:divetime|preceding-sibling::u1:divetime"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>

          <xsl:attribute name="value">
            <xsl:call-template name="gasConvert">
              <xsl:with-param name="mix">
                <xsl:value-of select="//gasdefinitions/mix[@id=$idx]/o2|//u:gasdefinitions/u:mix[@id=$idx]/u:o2|//u1:gasdefinitions/u1:mix[@id=$idx]/u1:o2"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>
        </event>
      </xsl:for-each>

      <xsl:for-each select="samples/waypoint|u:samples/u:waypoint|u1:samples/u1:waypoint">
        <sample>
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="divetime|u:divetime|u1:divetime"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>

          <xsl:if test="depth != ''">
            <xsl:attribute name="depth">
              <xsl:value-of select="concat(depth, ' m')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="u:depth|u1:depth != ''">
            <xsl:attribute name="depth">
              <xsl:value-of select="concat(u:depth|u1:depth, ' m')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="temperature != '' and $temperatureSamples &gt; 0">
            <xsl:attribute name="temperature">
              <xsl:value-of select="concat(format-number(temperature - 273.15, '0.0'), ' C')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="u:temperature|u1:temperature != '' and $temperatureSamples &gt; 0">
            <xsl:attribute name="temperature">
              <xsl:value-of select="concat(format-number(u:temperature|u1:temperature - 273.15, '0.0'), ' C')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="otu|u:otu|u1:otu &gt; 0">
            <xsl:attribute name="otu">
              <xsl:value-of select="otu|u:otu|u1:otu"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="cns|u:cns|u1:cns &gt; 0">
            <xsl:attribute name="cns">
              <xsl:value-of select="concat(cns|u:cns|u1:cns, ' C')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="setpo2|u:setpo2|u1:setpo2 != ''">
            <xsl:attribute name="po2">
              <xsl:call-template name="convertPascal">
                <xsl:with-param name="value">
                  <xsl:value-of select="setpo2|u:setpo2|u1:setpo2"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
        </sample>
      </xsl:for-each>
    </dive>
  </xsl:template>

  <!-- convert time in seconds to minutes:seconds -->
  <xsl:template name="timeConvert">
    <xsl:param name="timeSec"/>
    <xsl:if test="$timeSec != ''">
      <xsl:value-of select="concat(floor(number($timeSec) div 60), ':',    format-number(floor(number($timeSec) mod 60), '00'), ' min')"/>
    </xsl:if>
  </xsl:template>
  <!-- end convert time -->

  <!-- convert gas -->
  <xsl:template name="gasConvert">
    <xsl:param name="mix"/>
    <xsl:if test="$mix != ''">
      <xsl:choose>
        <xsl:when test="$mix &gt;= 0">
          <xsl:choose>
            <xsl:when test="$mix &gt; 1">
              <xsl:value-of select="concat($mix, '%')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat($mix * 100, '%')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$mix"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>
  <!-- end convert time -->

</xsl:stylesheet>
