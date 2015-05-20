<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <divelog program='subsurface-import' version='2'>
      <settings>
        <xsl:for-each select="/dives/dive">
          <divecomputerid deviceid="ffffffff">
            <xsl:attribute name="model">
              <xsl:value-of select="computer"/>
            </xsl:attribute>
            <xsl:attribute name="serial">
              <xsl:value-of select="serial"/>
            </xsl:attribute>
          </divecomputerid>
        </xsl:for-each>
      </settings>
      <dives>
        <xsl:apply-templates select="/dives/dive"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="dive">
    <xsl:variable name="units" select="/dives/units"/>

    <!-- Count the amount of temeprature samples during the dive -->
    <xsl:variable name="temperatureSamples">
      <xsl:call-template name="temperatureSamples" select="/dives/dive/samples">
        <xsl:with-param name="units" select="$units"/>
      </xsl:call-template>
    </xsl:variable>

    <!-- Count the amount of pressure samples during the dive -->
    <xsl:variable name="pressureSamples">
      <xsl:call-template name="pressureSamples" select="/dives/dive/samples"/>
    </xsl:variable>

    <dive>
      <xsl:attribute name="number">
        <xsl:choose>
          <xsl:when test="divenumber != ''">
            <xsl:value-of select="divenumber"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="diveNumber"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>

      <xsl:if test="rating != ''">
        <xsl:attribute name="rating">
          <xsl:value-of select="rating"/>
        </xsl:attribute>
      </xsl:if>

      <xsl:attribute name="date">
        <xsl:value-of select="date"/>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:call-template name="timeConvert">
          <xsl:with-param name="timeSec" select="duration"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:attribute name="tags">
        <xsl:for-each select="tags/tag|entryType">
          <xsl:choose>
            <xsl:when test="position() = 1">
              <xsl:value-of select="."/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat(',', .)"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
      </xsl:attribute>

      <xsl:variable name="delta">
        <xsl:value-of select="sampleInterval"/>
      </xsl:variable>

      <xsl:choose>
        <xsl:when test="site/country|site/location|site/name|site/lat|site/lon">
          <location debug="true">
            <xsl:for-each select="site/country|site/location|site/name">
              <xsl:choose>
                <xsl:when test="following-sibling::location[1] != '' and . != ''">
                  <xsl:value-of select="concat(., ' / ')"/>
                </xsl:when>
                <xsl:when test="following-sibling::name[1] != '' and . != ''">
                  <xsl:value-of select="concat(., ' / ')"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="."/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:for-each>
          </location>

      <!-- This will discard GPS coordinates of 0 0 but I suppose that
           is better than all non-gps dives to be in that location -->
          <xsl:if test="site/lat != 0">
            <gps>
              <xsl:value-of select="concat(site/lat, ' ', site/lon)"/>
            </gps>
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <location>
            <xsl:for-each select="country|location|site">
              <xsl:choose>
                <xsl:when test="following-sibling::location[1] != '' and . != ''">
                  <xsl:value-of select="concat(., ' / ')"/>
                </xsl:when>
                <xsl:when test="following-sibling::site[1] != '' and . != ''">
                  <xsl:value-of select="concat(., ' / ')"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="."/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:for-each>
          </location>

      <!-- This will discard GPS coordinates of 0 0 but I suppose that
           is better than all non-gps dives to be in that location -->
          <xsl:if test="sitelat != 0">
            <gps>
              <xsl:value-of select="concat(sitelat, ' ', sitelon)"/>
            </gps>
          </xsl:if>
          <xsl:if test="siteLat != 0">
            <gps>
              <xsl:value-of select="concat(siteLat, ' ', siteLon)"/>
            </gps>
          </xsl:if>
        </xsl:otherwise>
      </xsl:choose>

      <notes>
        <xsl:value-of select="notes"/>
      </notes>

      <xsl:if test="o2percent != ''">
        <cylinder>
          <xsl:attribute name="o2">
            <xsl:value-of select="concat(o2percent, '%')"/>
          </xsl:attribute>
        </cylinder>
      </xsl:if>

      <xsl:for-each select="gases/gas">
        <cylinder>
          <xsl:if test="oxygen != ''">
            <xsl:attribute name="o2">
              <xsl:value-of select="concat(oxygen, '%')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="helium != ''">
            <xsl:attribute name="he">
              <xsl:value-of select="concat(helium, '%')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="pressureStart &gt; 0">
            <xsl:attribute name="start">
              <xsl:call-template name="pressureConvert">
                <xsl:with-param name="number" select="pressureStart"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="pressureEnd &gt; 0">
            <xsl:attribute name="end">
              <xsl:call-template name="pressureConvert">
                <xsl:with-param name="number" select="pressureEnd"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="tankSize &gt; 0">
            <xsl:attribute name="size">
              <xsl:call-template name="sizeConvert">
                <xsl:with-param name="singleSize" select="tankSize"/>
                <xsl:with-param name="double" select="double"/>
                <xsl:with-param name="pressure" select="workingPressure"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="workingPressure &gt; 0">
            <xsl:attribute name="workpressure">
              <xsl:call-template name="pressureConvert">
                <xsl:with-param name="number" select="workingPressure"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="tankName != ''">
            <xsl:attribute name="description">
              <xsl:value-of select="tankName"/>
            </xsl:attribute>
          </xsl:if>
        </cylinder>
      </xsl:for-each>

      <xsl:for-each select="gases/gas">
        <event name="gaschange">
          <xsl:attribute name="time">
            <xsl:call-template name="sec2time">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="sum(preceding-sibling::gas/duration)"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="value">
            <xsl:value-of select="helium * 65536 + oxygen"/>
          </xsl:attribute>
        </event>
      </xsl:for-each>

      <xsl:if test="diveMaster">
        <divemaster>
          <xsl:value-of select="diveMaster"/>
        </divemaster>
      </xsl:if>
      <buddy>
        <xsl:for-each select="buddies/buddy">
          <xsl:choose>
            <xsl:when test="following-sibling::*[1] != ''">
              <xsl:value-of select="concat(., ', ')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="."/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
      </buddy>

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
          <xsl:value-of select="computer"/>
        </xsl:attribute>

        <xsl:choose>
          <xsl:when test="maxdepth != ''">
            <depth>
              <xsl:attribute name="max">
                <xsl:call-template name="depthConvert">
                  <xsl:with-param name="depth">
                    <xsl:value-of select="maxdepth"/>
                  </xsl:with-param>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
              <xsl:attribute name="mean">
                <xsl:call-template name="depthConvert">
                  <xsl:with-param name="depth">
                    <xsl:value-of select="avgdepth"/>
                  </xsl:with-param>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
            </depth>
          </xsl:when>
          <xsl:otherwise>
            <depth>
              <xsl:attribute name="max">
                <xsl:call-template name="depthConvert">
                  <xsl:with-param name="depth">
                    <xsl:value-of select="maxDepth"/>
                  </xsl:with-param>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
              <xsl:attribute name="mean">
                <xsl:call-template name="depthConvert">
                  <xsl:with-param name="depth">
                    <xsl:value-of select="averageDepth"/>
                  </xsl:with-param>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
            </depth>
          </xsl:otherwise>
        </xsl:choose>

        <temperature>

          <!-- If we have temperature reading and it is non-zero, we use
             it. If the temperature is zero, we only use it if we have
             temperature samples from the dive. -->
          <xsl:if test="tempAir != ''">
            <xsl:variable name="air">
              <xsl:call-template name="tempConvert">
                <xsl:with-param name="temp" select="tempAir"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:variable>
            <xsl:if test="substring-before($air, ' ') != 0 or $temperatureSamples &gt; 0">
              <xsl:attribute name="air">
                <xsl:value-of select="$air"/>
              </xsl:attribute>
            </xsl:if>
          </xsl:if>

          <xsl:if test="tempLow != ''">
            <xsl:variable name="water">
              <xsl:call-template name="tempConvert">
                <xsl:with-param name="temp" select="tempLow"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:variable>
            <xsl:if test="substring-before($water, ' ') != 0 or $temperatureSamples &gt; 0">
              <xsl:attribute name="water">
                <xsl:value-of select="$water"/>
              </xsl:attribute>
            </xsl:if>
          </xsl:if>

          <xsl:if test="tempair != ''">
            <xsl:variable name="air">
              <xsl:call-template name="tempConvert">
                <xsl:with-param name="temp" select="tempair"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:variable>
            <xsl:if test="substring-before($air, ' ') != 0 or $temperatureSamples &gt; 0">
              <xsl:attribute name="air">
                <xsl:value-of select="$air"/>
              </xsl:attribute>
            </xsl:if>
          </xsl:if>
          <xsl:if test="templow != ''">
            <xsl:variable name="water">
              <xsl:call-template name="tempConvert">
                <xsl:with-param name="temp" select="temlow"/>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:variable>
            <xsl:if test="substring-before($water, ' ') != 0 or $temperatureSamples &gt; 0">
              <xsl:attribute name="water">
                <xsl:value-of select="$water"/>
              </xsl:attribute>
            </xsl:if>
          </xsl:if>
        </temperature>

        <xsl:if test="current != ''">
          <extradata key="current" value="{current}"/>
        </xsl:if>

        <xsl:if test="surfaceConditions != ''">
          <extradata key="surfaceConditions" value="{surfaceConditions}"/>
        </xsl:if>

        <xsl:if test="entryType != ''">
          <extradata key="entryType" value="{entryType}"/>
        </xsl:if>

        <xsl:for-each select="samples/sample">
          <sample>
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="time"/>
                </xsl:with-param>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:attribute name="depth">
              <xsl:call-template name="depthConvert">
                <xsl:with-param name="depth">
                  <xsl:value-of select="depth"/>
                </xsl:with-param>
                <xsl:with-param name="units" select="$units"/>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:if test="pressure != '' and $pressureSamples &gt; 0">
              <xsl:attribute name="pressure">
                <xsl:call-template name="pressureConvert">
                  <xsl:with-param name="number" select="pressure"/>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="temperature != '' and $temperatureSamples &gt; 0">
              <xsl:attribute name="temp">
                <xsl:call-template name="tempConvert">
                  <xsl:with-param name="temp" select="temperature"/>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
            </xsl:if>
          </sample>

          <xsl:if test="alarm != '' and alarm != gas_change">
            <event>
              <xsl:attribute name="time">
                <xsl:call-template name="timeConvert">
                  <xsl:with-param name="timeSec">
                    <xsl:value-of select="time"/>
                  </xsl:with-param>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
              <xsl:attribute name="name">
                <xsl:choose>
                  <xsl:when test="alarm = 'attention'">
                    <xsl:value-of select="'violation'"/>
                  </xsl:when>
                  <xsl:when test="alarm = 'ascent_rate'">
                    <xsl:value-of select="'ascent'"/>
                  </xsl:when>
                  <xsl:when test="alarm = 'deep_stop'">
                    <xsl:value-of select="'deepstop'"/>
                  </xsl:when>
                  <xsl:when test="alarm = 'deco'">
                    <xsl:value-of select="'deco stop'"/>
                  </xsl:when>
                  <xsl:when test="alarm = 'po2_high'">
                    <xsl:value-of select="'PO2'"/>
                  </xsl:when>
                  <xsl:when test="alarm = 'tissue_warning'">
                    <xsl:value-of select="'tissue level warning'"/>
                  </xsl:when>
                  <xsl:when test="alarm = 'user_depth_alarm'">
                    <xsl:value-of select="'maxdepth'"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="alarm"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:attribute>
            </event>
          </xsl:if>
        </xsl:for-each>
      </divecomputer>

    </dive>
  </xsl:template>

  <!-- convert pressure to bars -->
  <xsl:template name="pressureConvert">
    <xsl:param name="number"/>
    <xsl:param name="units"/>

    <xsl:choose>
      <xsl:when test="$units = 'Imperial'">
        <xsl:value-of select="concat(format-number(($number div 14.5037738007), '#.##'), ' bar')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($number, ' bar')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end convert pressure -->

  <!-- convert cuft to litres -->
  <xsl:template name="sizeConvert">
    <xsl:param name="singleSize"/>
    <xsl:param name="double"/>
    <xsl:param name="pressure"/>
    <xsl:param name="units"/>

    <xsl:variable name="size">
      <xsl:value-of select="format-number($singleSize + $singleSize * $double, '#.##')"/>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$units = 'Imperial'">
        <xsl:if test="$pressure != '0'">
          <xsl:value-of select="concat(format-number((($size * 14.7 div $pressure) div 0.035315), '#.##'), ' l')"/>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($size, ' l')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end convert pressure -->

  <!-- convert temperature from F to C -->
  <xsl:template name="tempConvert">
    <xsl:param name="temp"/>
    <xsl:param name="units"/>

    <xsl:choose>
      <xsl:when test="$units = 'Imperial'">
        <xsl:if test="$temp != ''">
          <xsl:value-of select="concat(format-number(($temp - 32) * 5 div 9, '0.0'), ' C')"/>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="$temp != ''">
          <xsl:value-of select="concat($temp, ' C')"/>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end convert temperature -->

  <!-- convert time in seconds to minutes:seconds -->
  <xsl:template name="timeConvert">
    <xsl:param name="timeSec"/>
    <xsl:param name="units"/>

    <xsl:if test="$timeSec != ''">
      <xsl:value-of select="concat(floor(number($timeSec) div 60), ':',    format-number(floor(number($timeSec) mod 60), '00'), ' min')"/>
    </xsl:if>
  </xsl:template>
  <!-- end convert time -->

  <!-- convert depth to meters -->
  <xsl:template name="depthConvert">
    <xsl:param name="depth"/>
    <xsl:param name="units"/>

    <xsl:choose>
      <xsl:when test="$units = 'Imperial'">
        <xsl:value-of select="concat(format-number(($depth * 0.3048), '#.##'), ' m')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($depth, ' m')"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- end convert depth -->

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

  <xsl:template name="pressureSamples">
    <xsl:value-of select="count(descendant::pressure[. &gt; 0])"/>
  </xsl:template>

</xsl:stylesheet>
