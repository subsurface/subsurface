<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <divelog program='subsurface-import' version='2'>
      <settings>
          <divecomputerid deviceid="ffffffff">
            <xsl:attribute name="model">
              <xsl:value-of select="'Mares'"/>
            </xsl:attribute>
            <xsl:attribute name="serial">
              <xsl:value-of select="''"/>
            </xsl:attribute>
          </divecomputerid>
      </settings>
      <divesites>
	      <xsl:for-each select="//locations/location">
		      <site>
			      <xsl:attribute name="uuid">
				      <xsl:value-of select="@id"/>
			      </xsl:attribute>
			      <xsl:attribute name="name">
				      <xsl:value-of select="title"/>
			      </xsl:attribute>
			      <!-- TODO: origin needs to be set properly -->
			      <geo cat='2' origin='0'>
				      <xsl:attribute name="value">
					      <xsl:value-of select="country"/>
				      </xsl:attribute>
			      </geo>
		      </site>
	      </xsl:for-each>
      </divesites>
      <dives>
	      <xsl:apply-templates select="/exportTrak/logbooks/logbook/dive"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="dive">
    <dive>
      <xsl:attribute name="number">
        <xsl:value-of select="number"/>
      </xsl:attribute>

      <xsl:attribute name="date">
        <xsl:value-of select="substring-before(start, 'T')"/>
      </xsl:attribute>

      <xsl:attribute name="time">
        <xsl:value-of select="substring-after(start, 'T')"/>
      </xsl:attribute>

          <xsl:variable name="ref">
		  <xsl:value-of select="@id"/>
          </xsl:variable>

      <xsl:if test="//locations/location[@id = $ref] != ''">
	      <xsl:attribute name="divesiteid">
		      <xsl:value-of select="$ref"/>
	      </xsl:attribute>
      </xsl:if>

      <notes>
        <xsl:value-of select="comment"/>
      </notes>

      <!-- Hardcoding units to metric for now as the only Mares log I have seen was in metric -->'
          <xsl:variable name="units">
		  <xsl:value-of select="'metric'"/>
          </xsl:variable>

      <xsl:if test="weight != ''">
        <weightsystem>
          <xsl:attribute name="weight">
            <xsl:call-template name="weightConvert">
              <xsl:with-param name="weight" select="translate(weight, ',', '.')"/>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="description">
            <xsl:value-of select="'unknown'"/>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

      <divecomputer deviceid="ffffffff">
        <xsl:attribute name="model">
          <xsl:value-of select="'Mares import'"/>
        </xsl:attribute>

	<xsl:if test="maxDepth != '' or avgDepth != ''">
		<depth>
			<xsl:if test="maxDepth != ''">
				<xsl:attribute name="max">
					<xsl:call-template name="depthConvert">
						<xsl:with-param name="depth">
							<xsl:value-of select="maxDepth"/>
						</xsl:with-param>
						<xsl:with-param name="units" select="$units"/>
					</xsl:call-template>
				</xsl:attribute>
			</xsl:if>
			<xsl:if test="avgDepth != ''">
				<xsl:attribute name="mean">
					<xsl:call-template name="depthConvert">
						<xsl:with-param name="depth">
							<xsl:value-of select="avgDepth"/>
						</xsl:with-param>
						<xsl:with-param name="units" select="$units"/>
					</xsl:call-template>
				</xsl:attribute>
			</xsl:if>
		</depth>
	</xsl:if>

        <temperature>
          <xsl:if test="airTemp != ''">
            <xsl:variable name="air">
              <xsl:call-template name="tempConvert">
                <xsl:with-param name="temp" select="airTemp"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:variable>
          </xsl:if>

          <xsl:if test="minTemp != ''">
            <xsl:variable name="water">
              <xsl:call-template name="tempConvert">
                <xsl:with-param name="temp" select="minTemp"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:variable>
          </xsl:if>
        </temperature>

	<xsl:for-each select="diveProfiles/diveProfile">
          <sample>
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="diveProfileTime div 1000"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:attribute name="depth">
              <xsl:call-template name="depthConvert">
                <xsl:with-param name="depth">
                  <xsl:value-of select="diveProfileDepth"/>
                </xsl:with-param>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:if test="diveProfileTemperature &gt; 0">
              <xsl:attribute name="temp">
                <xsl:call-template name="tempConvert">
                  <xsl:with-param name="temp" select="diveProfileTemperature"/>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
            </xsl:if>
          </sample>

        </xsl:for-each>
      </divecomputer>

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

  <!-- convert weight to kg -->
  <xsl:template name="weightConvert">
    <xsl:param name="weight"/>
    <xsl:param name="units"/>

    <xsl:choose>
      <xsl:when test="$weight &gt; 0">
        <xsl:choose>
          <xsl:when test="$units = 'Imperial'">
            <xsl:value-of select="concat(format-number(($weight * 0.453592), '#.##'), ' kg')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($weight, ' kg')"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$weight"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end convert weight -->

</xsl:stylesheet>
