<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:u="http://www.streit.cc/uddf/3.2/"
  xmlns:u1="http://www.streit.cc/uddf/3.1/"
  exclude-result-prefixes="u u1"
  version="1.0">
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <divelog program="subsurface-import" version="2">
      <settings>
        <divecomputerid deviceid="ffffffff">
          <xsl:choose>
            <xsl:when test="/UDDF/history != ''">
              <xsl:apply-templates select="/UDDF/history"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="/uddf/generator|/u:uddf/u:generator|/u1:uddf/u1:generator"/>
            </xsl:otherwise>
          </xsl:choose>
        </divecomputerid>
      </settings>
      <dives>
        <xsl:apply-templates select="/uddf/profiledata/repetitiongroup/dive|/u:uddf/u:profiledata/u:repetitiongroup/u:dive|/u1:uddf/u1:profiledata/u1:repetitiongroup/u1:dive|/UDDF/dive"/>
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

  <xsl:template match="modified">
    <xsl:if test="application/name != ''">
      <xsl:attribute name="model">
        <xsl:value-of select="application/name"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="application/version != ''">
      <xsl:attribute name="serial">
        <xsl:value-of select="application/version"/>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

  <xsl:template match="dive|u:dive|u1:dive">
    <dive>
      <!-- Count the amount of temeprature samples during the dive -->
      <xsl:variable name="temperatureSamples">
        <xsl:call-template name="temperatureSamples" select="samples/waypoint/temperature|u:samples/u:waypoint/u:temperature|u1:samples/u1:waypoint/u1:temperature|samples/t">
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
      <xsl:if test="dive_number != ''">
        <xsl:attribute name="number">
          <xsl:value-of select="dive_number"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:if test="dive_duration != '' and dive_duration != 0">
        <xsl:attribute name="duration">
          <xsl:call-template name="timeConvert">
            <xsl:with-param name="timeSec">
              <xsl:value-of select="dive_duration"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:if>

      <xsl:if test="condition/visibility|informationafterdive/visibility != '' and condition/visibility|informationafterdive/visibility != 0">
        <xsl:attribute name="visibility">
          <xsl:choose>
            <xsl:when test="condition/visibility|informationafterdive/visibility &lt; 1">
              <xsl:value-of select="1"/>
            </xsl:when>
            <xsl:when test="condition/visibility|informationafterdive/visibility &lt;= 3">
              <xsl:value-of select="2"/>
            </xsl:when>
            <xsl:when test="condition/visibility|informationafterdive/visibility &lt;= 5">
              <xsl:value-of select="3"/>
            </xsl:when>
            <xsl:when test="condition/visibility|informationafterdive/visibility &lt;= 10">
              <xsl:value-of select="4"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="5"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </xsl:if>

      <xsl:if test="informationafterdive/rating/ratingvalue != '' and informationafterdive/rating/ratingvalue != 0">
        <xsl:attribute name="rating">
          <xsl:choose>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt; 2">
              <xsl:value-of select="0"/>
            </xsl:when>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt;= 2">
              <xsl:value-of select="1"/>
            </xsl:when>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt;= 4">
              <xsl:value-of select="2"/>
            </xsl:when>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt;= 6">
              <xsl:value-of select="3"/>
            </xsl:when>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt;= 8">
              <xsl:value-of select="4"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="5"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </xsl:if>

      <xsl:if test="condition/air_temp|informationbeforedive/airtemperature != ''">
        <divetemperature>
          <xsl:attribute name="air">
            <xsl:value-of select="concat(format-number(condition/air_temp|informationbeforedive/airtemperature - 273.15, '0.0'), ' C')"/>
          </xsl:attribute>
        </divetemperature>
      </xsl:if>

      <xsl:if test="dive_site_ref/@ref|informationbeforedive/dive_site_ref/@ref != ''">
        <location>
          <xsl:variable name="ref">
            <xsl:value-of select="dive_site_ref/@ref|informationbeforedive/dive_site_ref/@ref"/>
          </xsl:variable>
          <xsl:if test="//dive_site[@id=$ref]/geography/gps/longitude != ''">
            <xsl:attribute name="gps">
              <xsl:value-of select="concat(//dive_site[@id=$ref]/geography/gps/latitude, ' ', //dive_site[@id=$ref]/geography/gps/longitude)"/>
            </xsl:attribute>
          <xsl:for-each select="//dive_site[@id=$ref]/geography/location|//dive_site[@id=$ref]/name">
            <xsl:value-of select="."/>
            <xsl:if test=". != '' and following-sibling::*[1]/* != ''"> / </xsl:if>
          </xsl:for-each>
          </xsl:if>
        </location>
      </xsl:if>

      <xsl:if test="buddy_ref/@ref|informationbeforedive/buddy_ref/@ref != ''">
        <buddy>
          <xsl:variable name="ref">
            <xsl:value-of select="buddy_ref/@ref|informationbeforedive/buddy_ref/@ref"/>
          </xsl:variable>
          <xsl:for-each select="//diver[@id=$ref]/personal/first_name|//diver[@id=$ref]/personal/nick_name|//diver[@id=$ref]/personal/family_name|//diver/buddy[@id=$ref]/personal/first_name|//diver/buddy[@id=$ref]/personal/nick_name|//diver/buddy[@id=$ref]/personal/family_name">
            <xsl:value-of select="."/>
            <xsl:if test=". != '' and (following-sibling::*[1] != '' or following-sibling::*[2] != '')"> / </xsl:if>
          </xsl:for-each>
        </buddy>
      </xsl:if>

      <xsl:if test="note/text|informationafterdive/notes/para != ''">
        <notes>
          <xsl:value-of select="note/text|informationafterdive/notes/para"/>
        </notes>
      </xsl:if>

      <xsl:if test="equipment_used/weight_used &gt; 0">
        <weightsystem description="unknown">
          <xsl:attribute name="weight">
            <xsl:value-of select="concat(format-number(equipment_used/weight_used, '0.0'), ' kg')"/>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

      <xsl:for-each select="lowesttemperature|informationafterdive/lowesttemperature|u:lowesttemperature|u:informationafterdive/u:lowesttemperature|u1:lowesttemperature|u1:informationafterdive/u1:lowesttemperature|condition/water_temp">
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
          <xsl:value-of select="/uddf/generator/name|/u:uddf/u:generator/u:name|/u1:uddf/u1:generator/u1:name|/UDDF/history/modified/application/name"/>
        </xsl:attribute>
      </divecomputer>

      <xsl:if test="equipment_used/tank_used != ''">
        <xsl:for-each select="equipment_used/tank_used">
          <cylinder>
            <xsl:variable name="idx">
              <xsl:value-of select="./tank_ref/@ref"/>
            </xsl:variable>
            <xsl:variable name="gas">
              <xsl:value-of select="./gas_ref/@ref"/>
            </xsl:variable>
            <xsl:attribute name="size">
              <xsl:value-of select="//equipment[@id=$idx]/tank/volume"/>
            </xsl:attribute>
            <xsl:attribute name="description">
              <xsl:value-of select="//equipment[@id=$idx]/general/name"/>
            </xsl:attribute>
            <xsl:attribute name="o2">
              <xsl:value-of select="//gas_mix[@id=$gas]/o2"/>
            </xsl:attribute>
            <xsl:attribute name="he">
              <xsl:value-of select="//gas_mix[@id=$gas]/he"/>
            </xsl:attribute>
            <xsl:attribute name="start">
              <xsl:value-of select="concat(substring-before(./pressure_start, '.') div 100000, ' bar')"/>
            </xsl:attribute>
            <xsl:attribute name="end">
              <xsl:value-of select="concat(substring-before(./pressure_end, '.') div 100000, ' bar')"/>
            </xsl:attribute>
          </cylinder>
        </xsl:for-each>
      </xsl:if>

      <xsl:for-each select="tankdata|u:tankdata|u1:tankdata">
        <cylinder>
          <xsl:variable name="gas">
            <xsl:value-of select="link/@ref|u:link/@ref|u1:link/@ref"/>
          </xsl:variable>

          <xsl:if test="tankvolume|u:tankvolume|u1:tankvolume != ''">
            <xsl:attribute name="size">
              <xsl:value-of select="tankvolume|u:tankvolume|u1:tankvolume"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:attribute name="o2">
            <xsl:value-of select="//gasdefinitions/mix[@id=$gas]/o2|//u:gasdefinitions/u:mix[@id=$gas]/u:o2|//u1:gasdefinitions/u1:mix[@id=$gas]/u1:o2"/>
          </xsl:attribute>
          <xsl:attribute name="he">
            <xsl:value-of select="//gasdefinitions/mix[@id=$gas]/he|//u:gasdefinitions/u:mix[@id=$gas]/u:he|//u1:gasdefinitions/u1:mix[@id=$gas]/u1:he"/>
          </xsl:attribute>

          <xsl:if test="tankpressurebegin|u:tankpressurebegin|u1:tankpressurebegin != ''">
            <xsl:attribute name="start">
              <xsl:value-of select="concat(format-number(tankpressurebegin|u:tankpressurebegin|u1:tankpressurebegin div 100000, '#.#'), ' bar')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="tankpressureend|u:tankpressureend|u1:tankpressureend != ''">
            <xsl:attribute name="end">
              <xsl:value-of select="concat(format-number(tankpressureend|u:tankpressureend|u1:tankpressureend div 100000, '#.#'), ' bar')"/>
            </xsl:attribute>
          </xsl:if>
        </cylinder>
      </xsl:for-each>

      <depth>
        <xsl:for-each select="greatestdepth|informationafterdive/greatestdepth|u:greatestdepth|u:informationafterdive/u:greatestdepth|u1:greatestdepth|u1:informationafterdive/u1:greatestdepth|max_depth">
          <xsl:attribute name="max">
            <xsl:value-of select="concat(format-number(., '0.00'), ' m')"/>
          </xsl:attribute>
        </xsl:for-each>
        <xsl:for-each select="averagedepth|informationafterdive/averagedepth|u:averagedepth|u:informationafterdive/u:averagedepth|u1:averagedepth|u1:informationafterdive/u1:averagedepth">
          <xsl:attribute name="mean">
            <xsl:value-of select="concat(format-number(., '0.00'), ' m')"/>
          </xsl:attribute>
        </xsl:for-each>
      </depth>

      <!-- Aquadivelog gas switches require more lookups than other UDDF
           formats I have seen -->
      <xsl:for-each select="samples/switch">
        <xsl:variable name="tank_idx">
          <xsl:value-of select="./@tank"/>
        </xsl:variable>
        <xsl:variable name="idx">
          <xsl:value-of select="../../equipment_used/tank_used[@id=$tank_idx]/gas_ref/@ref"/>
        </xsl:variable>

        <xsl:variable name="type">
          <xsl:choose>
            <xsl:when test="translate(//gas_def/gas_mix[@id=$idx]/o2, ',', '.') &gt; 0">
              <xsl:value-of select="25"/> <!-- SAMPLE_EVENT_GASCHANGE2 -->
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="11"/> <!-- SAMPLE_EVENT_GASCHANGE -->
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <event name="gaschange" type="{$type}">
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="following-sibling::t"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>

          <xsl:attribute name="value">
            <xsl:call-template name="gasConvert">
              <xsl:with-param name="mix">
                <xsl:value-of select="translate(//gas_def/gas_mix[@id=$idx]/o2, ',', '.')"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>
        </event>
      </xsl:for-each>

      <!-- Other gas switches than Aquadivelog -->
      <xsl:for-each select="samples/waypoint/switchmix|u:samples/u:waypoint/u:switchmix|u1:samples/u1:waypoint/u1:switchmix">
			<!-- Index to lookup gas per cent -->
        <xsl:variable name="idx">
          <xsl:value-of select="./@ref"/>
        </xsl:variable>

        <xsl:variable name="type">
          <xsl:choose>
            <xsl:when test="translate(//gasdefinitions/mix[@id=$idx]/he|//u:gasdefinitions/u:mix[@id=$idx]/u:he|//u1:gasdefinitions/u1:mix[@id=$idx]/u1:he, ',', '.') &gt; 0">
              <xsl:value-of select="25"/> <!-- SAMPLE_EVENT_GASCHANGE2 -->
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="11"/> <!-- SAMPLE_EVENT_GASCHANGE -->
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <event name="gaschange" type="{$type}">
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
                <xsl:value-of select="translate(//gasdefinitions/mix[@id=$idx]/o2|//u:gasdefinitions/u:mix[@id=$idx]/u:o2|//u1:gasdefinitions/u1:mix[@id=$idx]/u1:o2, ',', '.')"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>
        </event>
      </xsl:for-each>

      <xsl:for-each select="samples/waypoint/alarm|u:samples/u:waypoint/u:alarm|u1:samples/u1:waypoint/u1:alarm">
        <event>
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="preceding-sibling::divetime|preceding-sibling::u:divetime|preceding-sibling::u1:divetime"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>

          <xsl:attribute name="name">
            <xsl:value-of select="."/>
          </xsl:attribute>
        </event>
      </xsl:for-each>

      <xsl:for-each select="samples/waypoint/heading|u:samples/u:waypoint/u:heading|u1:samples/u1:waypoint/u1:heading">
        <event name="heading">
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="preceding-sibling::divetime|preceding-sibling::u:divetime|preceding-sibling::u1:divetime"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>

          <xsl:attribute name="value">
            <xsl:value-of select="."/>
          </xsl:attribute>
        </event>
      </xsl:for-each>

      <xsl:for-each select="samples/waypoint|u:samples/u:waypoint|u1:samples/u1:waypoint|samples/d">
        <sample>
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="divetime|u:divetime|u1:divetime|preceding-sibling::t[1]"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>

          <xsl:choose>
            <xsl:when test="depth != ''">
              <xsl:attribute name="depth">
                <xsl:value-of select="concat(format-number(depth, '0.00'), ' m')"/>
              </xsl:attribute>
            </xsl:when>
            <xsl:when test="u:depth|u1:depth != ''">
              <xsl:attribute name="depth">
                <xsl:value-of select="concat(format-number(u:depth|u1:depth, '0.00'), ' m')"/>
              </xsl:attribute>
            </xsl:when>
            <xsl:when test=". != 0">
              <xsl:attribute name="depth">
                <xsl:value-of select="concat(format-number(., '0.00'), ' m')"/>
              </xsl:attribute>
            </xsl:when>
          </xsl:choose>

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

          <xsl:if test="tankpressure|u:tankpressure|u1:tankpressure != ''">
            <xsl:attribute name="pressure">
              <xsl:value-of select="concat(format-number(tankpressure|u:tankpressure|u1:tankpressure div 100000, '0.0'), ' bar')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="otu|u:otu|u1:otu &gt; 0">
            <xsl:attribute name="otu">
              <xsl:value-of select="otu|u:otu|u1:otu"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="cns|u:cns|u1:cns &gt; 0">
            <xsl:attribute name="cns">
              <xsl:value-of select="cns|u:cns|u1:cns"/>
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

          <xsl:if test="nodecotime|u:nodecotime|u1:nodecotime &gt; 0">
            <xsl:attribute name="ndl">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="nodecotime|u:nodecotime|u1:nodecotime"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="decostop|u:decostop|u1:decostop">
            <xsl:attribute name="stoptime">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="decostop/@duration|u:decostop/@duration|u1:decostop/@duration"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:attribute name="stopdepth">
              <xsl:value-of select="decostop/@decodepth|u:decostop/@decodepth|u1:decostop/@decodepth"/>
            </xsl:attribute>
            <xsl:attribute name="in_deco">
              <xsl:choose>
                <xsl:when test="decostop/@kind|u:decostop/@kind|u1:decostop/@kind != 'mandatory'">
                  <xsl:value-of select="0"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="1"/>
                </xsl:otherwise>
              </xsl:choose>
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
