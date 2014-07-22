<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <divelog program='subsurface-import' version='2'>
      <dives>
        <xsl:apply-templates select="/Divinglog/Logbook/Dive"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="Dive">
    <dive>
      <xsl:attribute name="number">
	      <xsl:value-of select="Number"/>
      </xsl:attribute>

      <xsl:if test="rating &gt; 0">
        <xsl:attribute name="rating">
          <xsl:value-of select="Rating"/>
        </xsl:attribute>
      </xsl:if>

      <xsl:attribute name="date">
        <xsl:value-of select="Divedate"/>
      </xsl:attribute>

      <xsl:attribute name="time">
        <xsl:value-of select="Entrytime"/>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:choose>
          <xsl:when test="string-length(Divetime) - string-length(translate(./Divetime, '.', '')) = 1">
            <xsl:value-of select="concat(substring-before(Divetime, '.'), ':', format-number((substring-after(Divetime, '.') * 60 div 100), '00'), ' min')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat(Divetime, ' min')"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>

      <depth>
        <xsl:if test="Depth != ''">
          <xsl:attribute name="max">
            <xsl:value-of select="concat(Depth, ' m')"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="DepthAvg != ''">
          <xsl:attribute name="mean">
            <xsl:value-of select="concat(DepthAvg, ' m')"/>
          </xsl:attribute>
        </xsl:if>
      </depth>

      <location>
        <xsl:for-each select="Country/@Name | City/@Name | Place/@Name">
          <xsl:if test="position() != 1"> / </xsl:if>
          <xsl:value-of select="."/>
        </xsl:for-each>
      </location>

      <xsl:if test="Place/Lat != ''">
        <gps>
          <xsl:value-of select="concat(Place/Lat, ' ', Place/Lon)"/>
        </gps>
      </xsl:if>

      <xsl:if test="Buddy/@Names != ''">
        <buddy>
          <xsl:value-of select="Buddy/@Names"/>
        </buddy>
      </xsl:if>

      <xsl:if test="Divemaster != ''">
        <divemaster>
          <xsl:value-of select="Divemaster"/>
        </divemaster>
      </xsl:if>

      <cylinder>
        <xsl:attribute name="description">
          <xsl:value-of select="Tanktype"/>
        </xsl:attribute>
        <xsl:attribute name="start">
          <xsl:value-of select="PresS"/>
        </xsl:attribute>

        <xsl:attribute name="end">
          <xsl:value-of select="PresE"/>
        </xsl:attribute>

        <xsl:attribute name="size">
          <xsl:choose>
            <xsl:when test="DblTank = 'False'">
              <xsl:value-of select="Tanksize"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="format-number(Tanksize * 2, '#.##')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>

        <xsl:if test="O2 != ''">
          <xsl:attribute name="o2">
            <xsl:value-of select="concat(O2, '%')"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="Gas != ''">
          <xsl:attribute name="o2">
            <xsl:value-of select="substring-after(substring-before(Gas, ')'), 'O2=')"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="He != ''">
          <xsl:attribute name="he">
            <xsl:value-of select="concat(He, '%')"/>
          </xsl:attribute>
        </xsl:if>
      </cylinder>

      <temperature>
        <xsl:if test="Airtemp != ''">
          <xsl:attribute name="air">
            <xsl:value-of select="concat(Airtemp, ' C')"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="Watertemp != ''">
          <xsl:attribute name="water">
            <xsl:value-of select="concat(Watertemp, ' C')"/>
          </xsl:attribute>
        </xsl:if>
      </temperature>

      <suit>
        <xsl:value-of select="Divesuit"/>
      </suit>

      <xsl:if test="Weight != ''">
        <weightsystem>
          <!-- Is weight always in kilograms? -->
          <xsl:attribute name="weight">
            <xsl:value-of select="concat(Weight, ' kg')"/>
          </xsl:attribute>
          <xsl:attribute name="description">
            <xsl:value-of select="'unknown'"/>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

      <notes>
        <xsl:value-of select="Comments"/>
      </notes>

      <!-- Trying to detect if depth samples are in Imperial or Metric
           units. This is based on an assumption that maximum depth of
           a dive is recorded in metric and if samples contain bigger
	   values, they must be imperial. However, we double the depth
	   for the test in some cases the maximum sample depth might be
	   a bit more than what is recorded as maximum depth for the
	   dive.
           -->

      <xsl:variable name="max">
        <xsl:for-each select="Profile/P/Depth">
          <xsl:sort select="." data-type="number" order="descending"/>
          <xsl:if test="position() = 1"><xsl:value-of select="."/></xsl:if>
        </xsl:for-each>
      </xsl:variable>

      <xsl:variable name="depthUnit">
        <xsl:choose>
          <xsl:when test="$max &gt; Depth * 2 + 1">
            <xsl:value-of select="'imperial'"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="'metric'"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <divecomputer>
        <xsl:if test="Computer != ''">
          <xsl:attribute name="model">
	    <xsl:value-of select="Computer"/>
          </xsl:attribute>
	</xsl:if>

      <xsl:for-each select="Profile/P">
        <sample>
          <xsl:attribute name="time">
            <xsl:value-of select="concat(floor(number(./@Time) div 60), ':', format-number(floor(number(./@Time) mod 60), '00'), ' min')"/>
          </xsl:attribute>
          <!-- This looks like pure guess work to figure out the unit -->
          <xsl:if test="Temp != ''">
            <xsl:attribute name="temp">
              <xsl:choose>
                <xsl:when test="Temp &gt; 32">
                  <xsl:value-of select="concat(format-number((Temp - 32) * 5 div 9, '0.0'), ' C')"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="concat(Temp, ' C')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
          <!-- How does this pressure information work? How do we know
               which pressure information is in use?
               Until further information, just grab "randomly" the first
               pressure reading -->
          <xsl:attribute name="pressure">
            <xsl:value-of select="Press1"/>
          </xsl:attribute>
          <xsl:attribute name="depth">
            <xsl:call-template name="depthConvert">
              <xsl:with-param name="depth">
                <xsl:value-of select="Depth"/>
              </xsl:with-param>
              <xsl:with-param name="depthUnit">
                <xsl:value-of select="$depthUnit"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>
        </sample>
      </xsl:for-each>

      </divecomputer>


    </dive>
  </xsl:template>

  <!-- convert depth to meters -->
  <xsl:template name="depthConvert">
    <xsl:param name="depth"/>
    <xsl:param name="depthUnit"/>

    <xsl:if test="$depth != ''">
      <xsl:choose>
        <xsl:when test="$depthUnit = 'imperial'">
          <xsl:value-of select="concat(format-number($depth div 3.2808, '##.#'), ' m')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat($depth, ' m')"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>
  <!-- end convert depth -->

</xsl:stylesheet>
