<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xt="http://www.jclark.com/xt"
  extension-element-prefixes="xt" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:param name="units" select="units"/>
  <xsl:output method="text" encoding="UTF-8"/>

  <xsl:variable name="fs" select="','"/>
  <xsl:variable name="quote" select="'&quot;'"/>
  <xsl:variable name="lf" select="'&#xa;'"/>

  <xsl:template name="EscapeQuotes">
    <xsl:param name="value"/>
    <xsl:choose>
      <xsl:when test="contains($value, $quote)">
        <xsl:value-of select="concat(substring-before($value, $quote), $quote, $quote)"/>
        <xsl:call-template name="EscapeQuotes">
          <xsl:with-param name="value" select="substring-after($value, $quote)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$value"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="CsvEscape">
    <xsl:param name="value"/>
    <xsl:choose>
      <xsl:when test="contains($value, $fs)">
        <xsl:value-of select="$quote"/>
        <xsl:call-template name="EscapeQuotes">
          <xsl:with-param name="value" select="$value"/>
        </xsl:call-template>
        <xsl:value-of select="$quote"/>
      </xsl:when>
      <xsl:when test="contains($value, $lf)">
        <xsl:value-of select="$quote"/>
        <xsl:call-template name="EscapeQuotes">
          <xsl:with-param name="value" select="$value"/>
        </xsl:call-template>
        <xsl:value-of select="$quote"/>
      </xsl:when>
      <xsl:when test="contains($value, $quote)">
        <xsl:value-of select="$quote"/>
        <xsl:call-template name="EscapeQuotes">
          <xsl:with-param name="value" select="$value"/>
        </xsl:call-template>
        <xsl:value-of select="$quote"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$value"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="/divelog/dives">
    <!-- Find the maximum number of cylinders used on a dive -->
    <xsl:variable name="cylinders">
      <xsl:for-each select="dive|trip/dive">
        <xsl:sort select="count(cylinder)" data-type="number" order="descending"/>
        <xsl:if test="position() = 1">
          <xsl:value-of select="count(cylinder)"/>
        </xsl:if>
      </xsl:for-each>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:call-template name="printHeaders">
          <xsl:with-param name="cylinders" select="$cylinders"/>
          <xsl:with-param name="volumeUnits" select="'cuft'"/>
          <xsl:with-param name="distanceUnits" select="'ft'"/>
          <xsl:with-param name="temperatureUnits" select="'F'"/>
          <xsl:with-param name="pressureUnits" select="'psi'"/>
          <xsl:with-param name="massUnits" select="'lbs'"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="printHeaders">
          <xsl:with-param name="cylinders" select="$cylinders"/>
          <xsl:with-param name="volumeUnits" select="'l'"/>
          <xsl:with-param name="distanceUnits" select="'m'"/>
          <xsl:with-param name="temperatureUnits" select="'C'"/>
          <xsl:with-param name="pressureUnits" select="'bar'"/>
          <xsl:with-param name="massUnits" select="'kg'"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>

    <xsl:apply-templates select="dive|trip/dive">
      <xsl:with-param name="cylinders" select="$cylinders"/>
    </xsl:apply-templates>
  </xsl:template>

  <!-- Suppress printing of divesite notes -->
  <xsl:template match="divesites/site/notes"/>

  <xsl:template match="dive">
    <xsl:param name="cylinders"/>

    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="@number"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="@date"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="@time"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="substring-before(@duration, ' ')"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value">
        <xsl:choose>
          <xsl:when test="$units = 1">
            <xsl:value-of select="format-number(substring-before(@sac, ' ') * 0.035315, '#.##')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="substring-before(@sac, ' ')"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:with-param>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:choose>
      <xsl:when test="divecomputer[1]/depth/@mean|divecomputer[1]/depth/@max != ''">
        <xsl:apply-templates select="divecomputer[1]/depth"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$fs"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="$fs"/>

    <!-- Dive mode -->
    <xsl:if test="divecomputer[1]/@dctype != ''">
      <xsl:call-template name="CsvEscape">
        <xsl:with-param name="value" select="divecomputer[1]/@dctype"/>
      </xsl:call-template>
    </xsl:if>
    <xsl:value-of select="$fs"/>

    <!-- Air temperature -->
    <xsl:choose>
      <xsl:when test="divetemperature/@air != ''">
        <xsl:call-template name="temperature">
          <xsl:with-param name="temp" select="divetemperature/@air"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="divecomputer[1]/temperature/@air != ''">
        <xsl:call-template name="temperature">
          <xsl:with-param name="temp" select="divecomputer[1]/temperature/@air"/>
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
    <xsl:value-of select="$fs"/>

    <!-- Water temperature -->
    <xsl:choose>
      <xsl:when test="divetemperature/@water != ''">
        <xsl:call-template name="temperature">
        <xsl:with-param name="temp" select="divetemperature/@water"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="divecomputer[1]/temperature/@water != ''">
        <xsl:call-template name="temperature">
          <xsl:with-param name="temp" select="divecomputer[1]/temperature/@water"/>
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
    <xsl:value-of select="$fs"/>

    <!-- Cylinders -->
    <xsl:for-each select="cylinder">
      <xsl:call-template name="CsvEscape">
        <xsl:with-param name="value">
          <xsl:choose>
            <xsl:when test="$units = 1">
              <xsl:value-of select="concat(format-number((substring-before(@size, ' ') div 14.7 * 3000) * 0.035315, '#.#'), '')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="substring-before(@size, ' ')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>

      <xsl:variable name="pressureAttribute" select="concat('pressure', position() - 1)"/>
      <xsl:call-template name="CsvEscape">
        <xsl:with-param name="value">
          <xsl:choose>
            <xsl:when test="@start != ''">
              <xsl:call-template name="ConvertPressure">
                <xsl:with-param name="pressureBar" select="@start"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:when test="substring-before(../divecomputer[1]/sample/@*[name() = $pressureAttribute], ' ') > 0">
              <xsl:call-template name="ConvertPressure">
                <xsl:with-param name="pressureBar" select="../divecomputer[1]/sample/@*[name() = $pressureAttribute]"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:when test="(position() = 1) and (substring-before(../divecomputer[1]/sample/@pressure, ' ') > 0)">
              <xsl:call-template name="ConvertPressure">
                <xsl:with-param name="pressureBar" select="../divecomputer[1]/sample/@pressure"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="''"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>

      <xsl:call-template name="CsvEscape">
        <xsl:with-param name="value">
          <xsl:choose>
            <xsl:when test="@end != ''">
              <xsl:call-template name="ConvertPressure">
                <xsl:with-param name="pressureBar" select="@end"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:when test="substring-before(../divecomputer[1]/sample[@*[name() = $pressureAttribute]][last()]/@*[name() = $pressureAttribute], ' ') > 0">
              <xsl:call-template name="ConvertPressure">
                <xsl:with-param name="pressureBar" select="../divecomputer[1]/sample[@*[name() = $pressureAttribute]][last()]/@*[name() = $pressureAttribute]"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:when test="(position() = 1) and (substring-before(../divecomputer[1]/sample[@pressure][last()]/@pressure, ' ') > 0)">
              <xsl:call-template name="ConvertPressure">
                <xsl:with-param name="pressureBar" select="../divecomputer[1]/sample[@pressure][last()]/@pressure"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="''"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>

      <xsl:call-template name="CsvEscape">
        <xsl:with-param name="value" select="substring-before(@o2, '%')"/>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>

      <xsl:call-template name="CsvEscape">
        <xsl:with-param name="value" select="substring-before(@he, '%')"/>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>
    </xsl:for-each>

    <xsl:if test="count(cylinder) &lt; $cylinders">
      <xsl:call-template name="printEmptyCylinders">
        <xsl:with-param name="count" select="$cylinders - count(cylinder)"/>
      </xsl:call-template>
    </xsl:if>

    <xsl:choose>
      <!-- Old location format -->
      <xsl:when test="location != ''">
        <xsl:apply-templates select="location"/>
        <xsl:if test="string-length(location) = 0">
          <xsl:value-of select="$fs"/>
        </xsl:if>
      </xsl:when>
      <!-- Format with dive site managemenet -->
      <xsl:otherwise>
        <xsl:call-template name="CsvEscape">
          <xsl:with-param name="value">
            <xsl:if test="string-length(@divesiteid) > 0">
              <xsl:variable name="uuid" select="@divesiteid"/>
              <xsl:value-of select="//divesites/site[@uuid = $uuid]/@name"/>
            </xsl:if>
          </xsl:with-param>
        </xsl:call-template>
        <xsl:value-of select="$fs"/>
        <xsl:call-template name="CsvEscape">
          <xsl:with-param name="value">
            <xsl:if test="string-length(@divesiteid) > 0">
              <xsl:variable name="uuid" select="@divesiteid"/>
              <xsl:value-of select="//divesites/site[@uuid = $uuid]/@gps"/>
            </xsl:if>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="divemaster"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="buddy"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="suit"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="@rating"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="@visibility"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="notes"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>

    <xsl:variable name="trimmedweightlist">
      <xsl:for-each select="weightsystem">
        <weight>
          <xsl:value-of select="substring-before(@weight, ' ')"/>
        </weight>
      </xsl:for-each>
    </xsl:variable>
    <xsl:if test="weightsystem">
      <xsl:call-template name="CsvEscape">
        <xsl:with-param name="value">
          <xsl:choose>
            <xsl:when test="$units = 1">
              <xsl:value-of select="concat(format-number((sum(xt:node-set($trimmedweightlist)/node()) div 0.453592), '#.##'), '')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat(sum(xt:node-set($trimmedweightlist)/node()), '')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
    <xsl:value-of select="$fs"/>

    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="@tags"/>
    </xsl:call-template>
    <xsl:value-of select="$lf"/>
  </xsl:template>

  <!-- Convert pressure values to the correct units -->
  <xsl:template name="ConvertPressure">
    <xsl:param name="pressureBar"/>
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat(format-number((substring-before($pressureBar, ' ') * 14.5037738007), '#'), '')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-before($pressureBar, ' ')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Convert distance values to the correct units -->
  <xsl:template name="ConvertDistance">
    <xsl:param name="distanceM"/>
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat(format-number((substring-before($distanceM, ' ') div 0.3048), '#.##'), '')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="format-number(substring-before($distanceM, ' '), '#.##')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Print the header -->
  <xsl:template name="printHeaders">
    <xsl:param name="cylinders"/>
    <xsl:param name="volumeUnits"/>
    <xsl:param name="distanceUnits"/>
    <xsl:param name="temperatureUnits"/>
    <xsl:param name="pressureUnits"/>
    <xsl:param name="massUnits"/>

    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'dive number'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'date'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'time'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'duration [min]'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
            <xsl:with-param name="value" select="concat('sac [', $volumeUnits, '/min]')"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
            <xsl:with-param name="value" select="concat('maxdepth [', $distanceUnits, ']')"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="concat('avgdepth [', $distanceUnits, ']')"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'mode'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="concat('airtemp [', $temperatureUnits, ']')"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="concat('watertemp [', $temperatureUnits, ']')"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>

    <!-- Print cylinder info according to the amount of cylinders in dive -->
    <xsl:call-template name="printCylinderHeaders">
      <xsl:with-param name="index" select="1"/>
      <xsl:with-param name="count" select="$cylinders"/>
      <xsl:with-param name="volumeUnits" select="$volumeUnits"/>
      <xsl:with-param name="pressureUnits" select="$pressureUnits"/>
    </xsl:call-template>

    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'location'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'gps'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'divemaster'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'buddy'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'suit'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'rating'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'visibility'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'notes'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="concat('weight [', $massUnits, ']')"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="'tags'"/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:value-of select="$lf"/>
  </xsl:template>

  <!-- Print the header for cylinders -->
  <xsl:template name="printCylinderHeaders">
    <xsl:param name="index"/>
    <xsl:param name="count"/>
    <xsl:param name="volumeUnits"/>
    <xsl:param name="pressureUnits"/>

    <xsl:if test="$index &lt;= $count">
      <xsl:call-template name="CsvEscape">
              <xsl:with-param name="value" select="concat('cylinder size (', $index, ') [', $volumeUnits, ']')"/>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>
      <xsl:call-template name="CsvEscape">
              <xsl:with-param name="value" select="concat('startpressure (', $index, ') [', $pressureUnits, ']')"/>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>
      <xsl:call-template name="CsvEscape">
              <xsl:with-param name="value" select="concat('endpressure (', $index, ') [', $pressureUnits, ']')"/>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>
      <xsl:call-template name="CsvEscape">
              <xsl:with-param name="value" select="concat('o2 (', $index, ') [%]')"/>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>
      <xsl:call-template name="CsvEscape">
              <xsl:with-param name="value" select="concat('he (', $index, ') [%]')"/>
      </xsl:call-template>
      <xsl:value-of select="$fs"/>

      <xsl:call-template name="printCylinderHeaders">
        <xsl:with-param name="index" select="$index + 1"/>
        <xsl:with-param name="count" select="$count"/>
        <xsl:with-param name="volumeUnits" select="$volumeUnits"/>
        <xsl:with-param name="pressureUnits" select="$pressureUnits"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Depth template -->
  <xsl:template match="divecomputer/depth">
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value">
        <xsl:call-template name="ConvertDistance">
          <xsl:with-param name="distanceM" select="@max"/>
        </xsl:call-template>
      </xsl:with-param>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value">
        <xsl:call-template name="ConvertDistance">
          <xsl:with-param name="distanceM" select="@mean"/>
        </xsl:call-template>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- Temperature template -->
  <xsl:template name="temperature">
    <xsl:param name="temp"/>

    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value">
        <xsl:choose>
          <xsl:when test="$units = 1">
            <xsl:if test="substring-before($temp, ' ') > 0">
              <xsl:value-of select="concat(format-number((substring-before($temp, ' ') * 1.8) + 32, '0.0'), '')"/>
            </xsl:if>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="substring-before($temp, ' ')"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- Fill in non-existent cylinder info -->
  <xsl:template name="printEmptyCylinders">
    <xsl:param name="count"/>
    <xsl:if test="$count > 0">
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="$fs"/>
      <xsl:value-of select="$fs"/>
      <xsl:call-template name="printEmptyCylinders">
        <xsl:with-param name="count" select="$count - 1"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Location template -->
  <xsl:template match="location">
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="."/>
    </xsl:call-template>
    <xsl:value-of select="$fs"/>
    <xsl:call-template name="CsvEscape">
      <xsl:with-param name="value" select="@gps"/>
    </xsl:call-template>
  </xsl:template>
</xsl:stylesheet>
