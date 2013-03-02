<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:u="http://www.streit.cc/uddf/3.2/"
  exclude-result-prefixes="u"
  version="1.0">
  <xsl:import href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <divelog program="subsurface" version="2">
      <settings>
        <divecomputerid deviceid="ffffffff">
          <xsl:apply-templates select="/uddf/generator|/u:uddf/u:generator"/>
        </divecomputerid>
      </settings>
      <dives>
        <xsl:apply-templates select="/uddf/profiledata/repetitiongroup/dive|/u:uddf/u:profiledata/u:repetitiongroup/u:dive"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="generator|u:generator">
    <xsl:if test="manufacturer/name|u:manufacturer/u:name != ''">
      <xsl:attribute name="model">
        <xsl:value-of select="manufacturer/name|/u:manufacturer/u:name"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="name|u:name != ''">
      <xsl:attribute name="firmware">
        <xsl:value-of select="name|u:name"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="version|u:version != ''">
      <xsl:attribute name="serial">
        <xsl:value-of select="version|u:version"/>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

  <xsl:template match="gasdefinitions|u:gasdefinitions">
    <xsl:for-each select="u:mix|mix">
      <cylinder>
        <xsl:attribute name="description">
          <xsl:value-of select="u:name|name"/>
        </xsl:attribute>

        <xsl:attribute name="o2">
          <xsl:call-template name="gasConvert">
            <xsl:with-param name="mix">
              <xsl:value-of select="u:o2|o2"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:attribute>

        <xsl:attribute name="he">
          <xsl:call-template name="gasConvert">
            <xsl:with-param name="mix">
              <xsl:value-of select="u:he|he"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:attribute>
      </cylinder>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="u:dive|dive">
    <dive>
      <xsl:choose>
        <xsl:when test="date != ''">
          <xsl:attribute name="date">
            <xsl:value-of select="concat(date/year,'-',format-number(date/month, '00'), '-', format-number(date/day, '00'))"/>
          </xsl:attribute>
          <xsl:attribute name="time">
            <xsl:value-of select="concat(format-number(time/hour, '00'), ':', format-number(time/minute, '00'))"/>
          </xsl:attribute>
        </xsl:when>
        <xsl:when test="informationbeforedive/datetime|u:informationbeforedive/u:datetime != ''">
          <xsl:call-template name="datetime">
            <xsl:with-param name="value">
              <xsl:value-of select="informationbeforedive/datetime|u:informationbeforedive/u:datetime"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>

      <xsl:for-each select="lowesttemperature|informationafterdive/lowesttemperature|u:lowesttemperature|u:informationafterdive/u:lowesttemperature">
        <temperature>
          <xsl:attribute name="water">
            <xsl:value-of select="concat(format-number(.- 273.15, '0.0'), ' C')"/>
          </xsl:attribute>
        </temperature>
      </xsl:for-each>

      <xsl:apply-templates select="/uddf/gasdefinitions|/u:uddf/u:gasdefinitions"/>
      <depth>
        <xsl:for-each select="greatestdepth|informationafterdive/greatestdepth|u:greatestdepth|u:informationafterdive/u:greatestdepth">
          <xsl:attribute name="max">
            <xsl:value-of select="concat(., ' m')"/>
          </xsl:attribute>
        </xsl:for-each>
        <xsl:for-each select="averagedepth|informationafterdive/averagedepth|u:averagedepth|u:informationafterdive/u:averagedepth">
          <xsl:attribute name="mean">
            <xsl:value-of select="concat(., ' m')"/>
          </xsl:attribute>
        </xsl:for-each>
      </depth>

      <xsl:for-each select="samples/waypoint/switchmix|u:samples/u:waypoint/u:switchmix">
			<!-- Index to lookup gas per cent -->
        <xsl:variable name="idx">
          <xsl:value-of select="./@ref"/>
        </xsl:variable>

        <event name="gaschange" type="11">
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="preceding-sibling::divetime|preceding-sibling::u:divetime"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>

          <xsl:attribute name="value">
            <xsl:call-template name="gasConvert">
              <xsl:with-param name="mix">
                <xsl:value-of select="//gasdefinitions/mix[@id=$idx]/o2|//u:gasdefinitions/u:mix[@id=$idx]/u:o2"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>
        </event>
      </xsl:for-each>

      <xsl:for-each select="samples/waypoint|u:samples/u:waypoint">
        <sample>
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="divetime|u:divetime"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>

          <xsl:if test="depth != ''">
            <xsl:attribute name="depth">
              <xsl:value-of select="concat(depth, ' m')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="u:depth != ''">
            <xsl:attribute name="depth">
              <xsl:value-of select="concat(u:depth, ' m')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="temperature != ''">
            <xsl:attribute name="temperature">
              <xsl:value-of select="concat(format-number(temperature - 273.15, '0.0'), ' C')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="u:temperature != ''">
            <xsl:attribute name="temperature">
              <xsl:value-of select="concat(format-number(u:temperature - 273.15, '0.0'), ' C')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="otu|u:otu &gt; 0">
            <xsl:attribute name="otu">
              <xsl:value-of select="otu|u:otu"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="cns|u:cns &gt; 0">
            <xsl:attribute name="cns">
              <xsl:value-of select="concat(cns|u:cns, ' C')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="setpo2|u:setpo2 != ''">
            <xsl:attribute name="po2">
              <xsl:call-template name="convertPascal">
                <xsl:with-param name="value">
                  <xsl:value-of select="setpo2|u:setpo2"/>
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
