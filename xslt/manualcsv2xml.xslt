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
  <xsl:param name="divemasterField" select="divemasterField"/>
  <xsl:param name="buddyField" select="buddyField"/>
  <xsl:param name="suitField" select="suitField"/>
  <xsl:param name="notesField" select="notesField"/>
  <xsl:param name="weightField" select="weightField"/>
  <xsl:param name="dateformat" select="dateformat"/>
  <xsl:param name="airtempField" select="airtempField"/>
  <xsl:param name="watertempField" select="watertempField"/>
  <xsl:param name="cylindersizeField" select="cylindersizeField"/>
  <xsl:param name="startpressureField" select="startpressureField"/>
  <xsl:param name="endpressureField" select="endpressureField"/>
  <xsl:param name="o2Field" select="o2Field"/>
  <xsl:param name="heField" select="heField"/>
  <xsl:output method="xml" encoding="utf-8" indent="yes"/>

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
    <xsl:with-param name="remaining" select="$remaining"/>
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
    <xsl:param name="remaining"/>

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

    <xsl:if test="number($number) = $number">
    <dive>
      <xsl:attribute name="date">
        <xsl:choose>
          <xsl:when test="$dateField >= 0">
            <xsl:variable name="indate">
              <xsl:call-template name="getFieldByIndex">
                <xsl:with-param name="index" select="$dateField"/>
                <xsl:with-param name="line" select="$line"/>
              </xsl:call-template>
            </xsl:variable>
            <xsl:variable name="separator">
              <xsl:choose>
                <xsl:when test="substring-before($indate, '.') != ''">
                  <xsl:value-of select="'.'"/>
                </xsl:when>
                <xsl:when test="substring-before($indate, '-') != ''">
                  <xsl:value-of select="'-'"/>
                </xsl:when>
                <xsl:when test="substring-before($indate, '/') != ''">
                  <xsl:value-of select="'/'"/>
                </xsl:when>
              </xsl:choose>
            </xsl:variable>
            <xsl:choose>
              <!-- dd.mm.yyyy -->
              <xsl:when test="$datefmt = 0">
                <xsl:value-of select="concat(substring-after(substring-after($indate, $separator), $separator), '-', substring-before(substring-after($indate, $separator), $separator), '-', substring-before($indate, $separator))"/>
              </xsl:when>
              <!-- mm.yy.yyyy -->
              <xsl:when test="$datefmt = 1">
                <xsl:value-of select="concat(substring-after(substring-after($indate, $separator), $separator), '-', substring-before($indate, $separator), '-', substring-before(substring-after($indate, $separator), $separator))"/>
              </xsl:when>
              <!-- yyyy.mm.dd -->
              <xsl:when test="$datefmt = 2">
                <xsl:value-of select="concat(substring-before($indate, $separator), '-', substring-before(substring-after($indate, $separator), $separator), '-', substring-after(substring-after($indate, $separator), $separator))"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="'1900-1-1'"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat(substring($date, 1, 4), '-', substring($date, 5, 2), '-', substring($date, 7, 2))"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>

      <xsl:attribute name="time">
        <xsl:choose>
          <xsl:when test="$timeField >= 0">
            <xsl:variable name="timef">
              <xsl:call-template name="getFieldByIndex">
                <xsl:with-param name="index" select="$timeField"/>
                <xsl:with-param name="line" select="$line"/>
              </xsl:call-template>
            </xsl:variable>
            <xsl:choose>
              <xsl:when test="contains($timef, 'AM')">
                <xsl:value-of select="concat(substring-before($timef, ':') mod 12, ':', translate(substring-after($timef, ':'), ' AM', ''))"/>
              </xsl:when>
              <xsl:when test="contains($timef, 'PM')">
                <xsl:value-of select="concat(substring-before($timef, ':') mod 12 + 12, ':', translate(substring-after($timef, ':'), ' PM', ''))"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$timef"/>
              </xsl:otherwise>
            </xsl:choose>
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
        <xsl:variable name="duration">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$durationField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:attribute name="duration">
          <xsl:choose>
            <xsl:when test="$durationfmt = 1">
              <xsl:value-of select="$duration * 60"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:choose>
                <xsl:when test="string-length(translate($duration, translate($duration, ':', ''), '')) = 2">
                  <xsl:value-of select="concat(substring-before($duration, ':') * 60 + substring-before(substring-after($duration, ':'), ':'), ':', substring-after(substring-after($duration, ':'), ':'))"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="$duration"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:otherwise>
          </xsl:choose>
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

      <divecomputer deviceid="ffffffff" model="csv" />

      <xsl:if test="$locationField &gt;= 0 or $gpsField &gt;= 0">
        <location>
          <xsl:if test="$gpsField &gt;= 0">
            <xsl:attribute name="gps">
              <xsl:call-template name="getFieldByIndex">
                <xsl:with-param name="index" select="$gpsField"/>
                <xsl:with-param name="line" select="$line"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$locationField &gt;= 0">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="$locationField"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:if>
        </location>
      </xsl:if>

      <xsl:if test="$airtempField &gt;= 0 or $watertempField &gt;= 0">
        <divetemperature>
          <xsl:if test="$airtempField &gt;= 0">
            <xsl:attribute name="air">
              <xsl:variable name="air">
                <xsl:call-template name="getFieldByIndex">
                  <xsl:with-param name="index" select="$airtempField"/>
                  <xsl:with-param name="line" select="$line"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$air"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="(translate(translate($air, translate($air, '1234567890,.', ''), ''), ',', '.') - 32) * 5 div 9"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$watertempField &gt;= 0">
            <xsl:attribute name="water">
              <xsl:variable name="water">
                <xsl:call-template name="getFieldByIndex">
                  <xsl:with-param name="index" select="$watertempField"/>
                  <xsl:with-param name="line" select="$line"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$water"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="(translate(translate($water, translate($water, '1234567890,.', ''), ''), ',', '.') - 32) * 5 div 9"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
        </divetemperature>
      </xsl:if>

      <xsl:if test="$cylindersizeField &gt; 0 or $startpressureField &gt; 0 or $endpressureField &gt; 0 or $o2Field &gt; 0 or $heField &gt; 0">
        <cylinder>
          <xsl:if test="$cylindersizeField &gt; 0">
            <xsl:attribute name="size">
              <xsl:variable name="size">
                <xsl:call-template name="getFieldByIndex">
                  <xsl:with-param name="index" select="$cylindersizeField"/>
                  <xsl:with-param name="line" select="$line"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$size"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="format-number((translate($size, translate($size, '0123456789', ''), '') * 14.7 div 3000) div 0.035315, '#.#')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$startpressureField &gt; 0">
            <xsl:attribute name="start">
              <xsl:variable name="start">
                <xsl:call-template name="getFieldByIndex">
                  <xsl:with-param name="index" select="$startpressureField"/>
                  <xsl:with-param name="line" select="$line"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$start"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="format-number($start div 14.5037738, '#.###')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$endpressureField &gt; 0">
            <xsl:attribute name="end">
              <xsl:variable name="end">
                <xsl:call-template name="getFieldByIndex">
                  <xsl:with-param name="index" select="$endpressureField"/>
                  <xsl:with-param name="line" select="$line"/>
                </xsl:call-template>
              </xsl:variable>
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$end"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="format-number($end div 14.5037738, '#.###')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$o2Field &gt; 0">
            <xsl:attribute name="o2">
              <xsl:call-template name="getFieldByIndex">
                <xsl:with-param name="index" select="$o2Field"/>
                <xsl:with-param name="line" select="$line"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$heField &gt; 0">
            <xsl:attribute name="he">
              <xsl:call-template name="getFieldByIndex">
                <xsl:with-param name="index" select="$heField"/>
                <xsl:with-param name="line" select="$line"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
        </cylinder>
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

      <xsl:if test="$divemasterField >= 0">
        <divemaster>
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$divemasterField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </divemaster>
      </xsl:if>

      <xsl:if test="$buddyField >= 0">
        <buddy>
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$buddyField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </buddy>
      </xsl:if>

      <xsl:if test="$suitField >= 0">
        <suit>
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$suitField"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </suit>
      </xsl:if>

      <xsl:if test="$notesField >= 0">
        <notes>
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$notesField"/>
            <xsl:with-param name="line" select="$line"/>
            <xsl:with-param name="remaining" select="$remaining"/>
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
                <xsl:value-of select="translate(translate($weight, translate($weight, '1234567890,.', ''), ''), ',', '.') div 2.2046"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

    </dive>
  </xsl:if>
  </xsl:template>

</xsl:stylesheet>
