<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xt="http://www.jclark.com/xt"
  extension-element-prefixes="xt" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:param name="units" select="units"/>
  <xsl:output method="text" encoding="UTF-8"/>

  <xsl:variable name="fs"><xsl:text>	</xsl:text></xsl:variable>
  <xsl:variable name="lf"><xsl:text>
</xsl:text></xsl:variable>

  <xsl:template match="/divelog/dives">
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat('&quot;dive number&quot;', $fs, '&quot;date&quot;', $fs, '&quot;time&quot;', $fs, '&quot;duration&quot;', $fs, '&quot;maxdepth (ft)&quot;', $fs, '&quot;avgdepth (ft)&quot;', $fs, '&quot;airtemp (F)&quot;', $fs, '&quot;watertemp (F)&quot;', $fs, '&quot;cylinder size (cuft)&quot;', $fs, '&quot;startpressure (psi)&quot;', $fs, '&quot;endpressure (psi)&quot;', $fs, '&quot;o2&quot;', $fs, '&quot;he&quot;', $fs, '&quot;location&quot;', $fs, '&quot;gps&quot;', $fs, '&quot;divemaster&quot;', $fs, '&quot;buddy&quot;', $fs, '&quot;suit&quot;', $fs, '&quot;rating&quot;', $fs, '&quot;visibility&quot;', $fs, '&quot;notes&quot;', $fs, '&quot;weight (lbs)&quot;', $fs, '&quot;tags&quot;')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&quot;dive number&quot;', $fs, '&quot;date&quot;', $fs, '&quot;time&quot;', $fs, '&quot;duration&quot;', $fs, '&quot;maxdepth (m)&quot;', $fs, '&quot;avgdepth (m)&quot;', $fs, '&quot;airtemp (C)&quot;', $fs, '&quot;watertemp (C)&quot;', $fs, '&quot;cylinder size (l)&quot;', $fs, '&quot;startpressure (bar)&quot;', $fs, '&quot;endpressure (bar)&quot;', $fs, '&quot;o2&quot;', $fs, '&quot;he&quot;', $fs, '&quot;location&quot;', $fs, '&quot;gps&quot;', $fs, '&quot;divemaster&quot;', $fs, '&quot;buddy&quot;', $fs, '&quot;suit&quot;', $fs, '&quot;rating&quot;', $fs, '&quot;visibility&quot;', $fs, '&quot;notes&quot;', $fs, '&quot;weight (kg)&quot;', $fs, '&quot;tags&quot;')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>
</xsl:text>
    <xsl:apply-templates select="dive|trip/dive"/>
  </xsl:template>

  <xsl:template match="divesites/site/notes"/>

  <xsl:template match="dive">
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@number"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@date"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@time"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@duration"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:choose>
      <xsl:when test="divecomputer[1]/depth/@mean|divecomputer[1]/depth/@max != ''">
        <xsl:apply-templates select="divecomputer[1]/depth"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;&quot;</xsl:text>
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;&quot;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="divetemperature/@air|divetemperature/@water != ''">
        <xsl:apply-templates select="divetemperature"/>
      </xsl:when>
      <xsl:when test="divecomputer[1]/temperature/@air|divecomputer[1]/temperature/@water != ''">
        <xsl:apply-templates select="divecomputer[1]/temperature"/>
      </xsl:when>
      <xsl:otherwise>
        <!-- empty air temperature -->
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;&quot;</xsl:text>
        <!-- water temperature -->
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;&quot;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="cylinder[1]/@start|cylinder[1]/@end != ''">
        <xsl:apply-templates select="cylinder[1]"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;</xsl:text>
        <xsl:choose>
          <xsl:when test="$units = 1">
            <xsl:value-of select="concat(format-number((substring-before(cylinder[1]/@size, ' ') div 14.7 * 3000) * 0.035315, '#.#'), '')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="substring-before(cylinder[1]/@size, ' ')"/>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>&quot;</xsl:text>
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;</xsl:text>
        <xsl:choose>
          <xsl:when test="$units = 1">
            <xsl:choose>
              <xsl:when test="divecomputer[1]/sample[@pressure]/@pressure &gt; 0">
                <xsl:value-of select="concat(format-number((substring-before(divecomputer[1]/sample[@pressure]/@pressure, ' ') * 14.5037738007), '#'), '')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="''"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="substring-before(divecomputer[1]/sample[@pressure]/@pressure, ' ')"/>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>&quot;</xsl:text>
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;</xsl:text>
        <xsl:choose>
          <xsl:when test="$units = 1">
            <xsl:choose>
              <xsl:when test="divecomputer[1]/sample[@pressure][last()]/@pressure &gt; 0">
                <xsl:value-of select="concat(format-number((substring-before(divecomputer[1]/sample[@pressure][last()]/@pressure, ' ') * 14.5037738007), '#'), '')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="''"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="substring-before(divecomputer[1]/sample[@pressure][last()]/@pressure, ' ')"/>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>&quot;</xsl:text>
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;</xsl:text>
        <xsl:value-of select="cylinder[1]/@o2"/>
        <xsl:text>&quot;</xsl:text>
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;</xsl:text>
        <xsl:value-of select="cylinder[1]/@he"/>
        <xsl:text>&quot;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <!-- Old location format -->
      <xsl:when test="location != ''">
        <xsl:apply-templates select="location"/>
        <xsl:if test="string-length(location) = 0">
          <xsl:value-of select="$fs"/>
          <xsl:text>&quot;&quot;</xsl:text>
          <xsl:value-of select="$fs"/>
          <xsl:text>&quot;&quot;</xsl:text>
        </xsl:if>
      </xsl:when>
      <!-- Format with dive site managemenet -->
      <xsl:otherwise>
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;</xsl:text>
        <xsl:if test="string-length(@divesiteid) &gt; 0">
          <xsl:variable name="uuid">
            <xsl:value-of select="@divesiteid" />
          </xsl:variable>
          <xsl:value-of select="//divesites/site[@uuid = $uuid]/@name"/>
        </xsl:if>
        <xsl:text>&quot;</xsl:text>
        <xsl:value-of select="$fs"/>
        <xsl:text>&quot;</xsl:text>
        <xsl:if test="string-length(@divesiteid) &gt; 0">
          <xsl:variable name="uuid">
            <xsl:value-of select="@divesiteid" />
          </xsl:variable>
          <xsl:value-of select="//divesites/site[@uuid = $uuid]/@gps"/>
        </xsl:if>
        <xsl:text>&quot;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates select="divemaster"/>
    <xsl:if test="string-length(divemaster) = 0">
      <xsl:value-of select="$fs"/>
      <xsl:text>&quot;&quot;</xsl:text>
    </xsl:if>
    <xsl:apply-templates select="buddy"/>
    <xsl:if test="string-length(buddy) = 0">
      <xsl:value-of select="$fs"/>
      <xsl:text>&quot;&quot;</xsl:text>
    </xsl:if>
    <xsl:apply-templates select="suit"/>
    <xsl:if test="string-length(suit) = 0">
      <xsl:value-of select="$fs"/>
      <xsl:text>&quot;&quot;</xsl:text>
    </xsl:if>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@rating"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@visibility"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:apply-templates select="notes"/>
    <xsl:if test="string-length(notes) = 0">
      <xsl:value-of select="$fs"/>
      <xsl:text>&quot;&quot;</xsl:text>
    </xsl:if>

    <xsl:variable name="trimmedweightlist">
      <xsl:for-each select="weightsystem">
        <weight>
          <xsl:value-of select="substring-before(@weight, ' ')"/>
        </weight>
      </xsl:for-each>
    </xsl:variable>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:if test="weightsystem">
      <xsl:choose>
        <xsl:when test="$units = 1">
          <xsl:value-of select="concat(format-number((sum(xt:node-set($trimmedweightlist)/node()) div 0.453592), '#.##'), '')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat(sum(xt:node-set($trimmedweightlist)/node()), '')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
    <xsl:text>&quot;</xsl:text>

    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@tags"/>
    <xsl:text>&quot;</xsl:text>

    <xsl:text>
</xsl:text>
  </xsl:template>
  <xsl:template match="divecomputer/depth">
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat(format-number((substring-before(@max, ' ') div 0.3048), '#.##'), '')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-before(@max, ' ')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat(format-number((substring-before(@mean, ' ') div 0.3048), '#.##'), '')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-before(@mean, ' ')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&quot;</xsl:text>
  </xsl:template>
  <xsl:template match="divetemperature|temperature">
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:if test="substring-before(@air, ' ') &gt; 0">
          <xsl:value-of select="concat(format-number((substring-before(@air, ' ') * 1.8) + 32, '0.0'), '')"/>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-before(@air, ' ')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:if test="substring-before(@water, ' ') &gt; 0">
          <xsl:value-of select="concat(format-number((substring-before(@water, ' ') * 1.8) + 32, '0.0'), '')"/>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-before(@water, ' ')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&quot;</xsl:text>
  </xsl:template>
  <xsl:template match="cylinder">
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat(format-number((substring-before(@size, ' ') div 14.7 * 3000) * 0.035315, '#.#'), '')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-before(@size, ' ')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat(format-number((substring-before(@start, ' ') * 14.5037738007), '#'), '')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-before(@start, ' ')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:choose>
      <xsl:when test="$units = 1">
        <xsl:value-of select="concat(format-number((substring-before(@end, ' ') * 14.5037738007), '#'), '')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring-before(@end, ' ')"/>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@o2"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@he"/>
    <xsl:text>&quot;</xsl:text>
  </xsl:template>
  <xsl:template match="location">
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="@gps"/>
    <xsl:text>&quot;</xsl:text>
  </xsl:template>
  <xsl:template match="divemaster">
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>&quot;</xsl:text>
  </xsl:template>
  <xsl:template match="buddy">
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>&quot;</xsl:text>
  </xsl:template>
  <xsl:template match="suit">
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>&quot;</xsl:text>
  </xsl:template>
  <xsl:template match="notes">
    <xsl:value-of select="$fs"/>
    <xsl:text>&quot;</xsl:text>
    <xsl:value-of select="translate(translate(., $fs, ' '), $lf, ' ')"/>
    <xsl:text>&quot;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
