<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:key name="DC" match="dive" use="concat(@ComputerID, ':', @Computer)"/>

  <xsl:template match="/">
    <divelog program='subsurface-import' version='2'>
      <settings>
        <!-- Using the serial number as device ID for now. Once we have
             a change to get some testing done, we can jump on using
             extension that provides sha1 function.

+  xmlns:crypto="http://exslt.org/crypto"
+  extension-element-prefixes="crypto"
+        <divecomputerid deviceid="{substring(crypto:sha1(concat(@ComputerID, ':', @Computer)), 1, 8)}">

-->

        <xsl:for-each select="logbook/dive[generate-id() = generate-id(key('DC',concat(@ComputerID, ':', @Computer))[1])]">
          <divecomputerid deviceid="{@ComputerID}">
            <xsl:attribute name="model">
              <xsl:value-of select="@Computer"/>
            </xsl:attribute>
            <xsl:attribute name="serial">
              <xsl:value-of select="@ComputerID"/>
            </xsl:attribute>
          </divecomputerid>
        </xsl:for-each>
      </settings>
      <dives>
        <xsl:apply-templates select="/logbook"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="dive">
    <xsl:variable name="units" select="/logbook/prefs/@units"/>

    <dive>
      <xsl:attribute name="number">
        <xsl:value-of select="@Number"/>
      </xsl:attribute>

      <xsl:attribute name="date">
        <xsl:value-of select="concat(substring-after(substring-after(substring-before(@DateTime, ' '), '/'), '/'), '-', substring-before(@DateTime, '/'), '-', substring-before(substring-after(@DateTime, '/'), '/'))"/>
      </xsl:attribute>

      <xsl:attribute name="time">
        <xsl:value-of select="substring-after(@DateTime, ' ')"/>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:call-template name="timeConvert">
          <xsl:with-param name="timeSec" select="@Duration"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:variable name="delta">
        <xsl:value-of select="@SampleInterval"/>
      </xsl:variable>

      <location>
        <xsl:value-of select="@Location"/>
        <xsl:if test="@Location != '' and @Site != ''">
          <xsl:text> / </xsl:text>
        </xsl:if>
        <xsl:value-of select="@Site"/>
      </location>

      <xsl:variable name="lf">
        <xsl:text>
</xsl:text>
      </xsl:variable>
      <notes>
        <xsl:value-of select="notes"/>
        <xsl:value-of select="concat($lf, 'Weather: ', @Weather, $lf, 'Visibility: ', @Visibility)"/>
        <xsl:if test="@Boat != '' and @Boat != ' '">
          <xsl:value-of select="concat($lf, 'Boat: ', @Boat)"/>
        </xsl:if>
      </notes>

      <cylinder>
        <xsl:if test="@StartPSI &gt; 0">
          <xsl:attribute name="start">
            <xsl:call-template name="pressureConvert">
              <xsl:with-param name="number" select="@StartPSI"/>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="@EndPSI &gt; 0">
          <xsl:attribute name="end">
            <xsl:call-template name="pressureConvert">
              <xsl:with-param name="number" select="@EndPSI"/>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="@TankSize &gt; 0">
          <xsl:attribute name="size">
            <xsl:call-template name="sizeConvert">
              <xsl:with-param name="singleSize" select="@TankSize"/>
              <xsl:with-param name="double" select="'0'"/>
              <xsl:with-param name="pressure" select="@WorkingPressure"/>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="@WorkingPressure &gt; 0">
          <xsl:attribute name="workpressure">
            <xsl:call-template name="pressureConvert">
              <xsl:with-param name="number" select="@WorkingPressure"/>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="@Tank != ''">
          <xsl:attribute name="description">
            <xsl:value-of select="@Tank"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="@NitroxEnrichment != '' and @NitroxEnrichment != 21">
          <xsl:attribute name="o2">
            <xsl:value-of select="concat(@NitroxEnrichment, '%')"/>
          </xsl:attribute>
        </xsl:if>
      </cylinder>

      <xsl:if test="@Weight != '' and @Weight != '0.0'">
        <weightsystem>
          <xsl:attribute name="weight">
            <xsl:call-template name="weightConvert">
              <xsl:with-param name="weight" select="translate(@Weight, ',', '.')"/>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="description">
            <xsl:value-of select="'unknown'"/>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

      <buddy>
        <xsl:value-of select="@DiveBuddy"/>
      </buddy>

      <divemaster>
        <xsl:value-of select="@DiveMaster"/>
      </divemaster>

      <divecomputer deviceid="{@ComputerID}">

        <xsl:attribute name="model">
          <xsl:value-of select="@Computer"/>
        </xsl:attribute>

        <extradata key="Sample Interval" value="{@SampleInterval}"/>

        <xsl:if test="@AltitudeMode != ''">
          <extradata key="Altitude Mode" value="{@AltitudeMode}"/>
        </xsl:if>

        <xsl:if test="@PersonalMode != ''">
          <extradata key="Personal Mode" value="{@PersonalMode}"/>
        </xsl:if>

      <depth>
        <xsl:attribute name="max">
          <xsl:call-template name="depthConvert">
            <xsl:with-param name="depth">
              <xsl:value-of select="@MaxDepth"/>
            </xsl:with-param>
            <xsl:with-param name="units" select="$units"/>
          </xsl:call-template>
        </xsl:attribute>
        <xsl:attribute name="mean">
          <xsl:call-template name="depthConvert">
            <xsl:with-param name="depth">
              <xsl:value-of select="@AverageDepth"/>
            </xsl:with-param>
            <xsl:with-param name="units" select="$units"/>
          </xsl:call-template>
        </xsl:attribute>
      </depth>


      <temperature>
        <xsl:attribute name="air">
          <xsl:call-template name="tempConvert">
            <xsl:with-param name="temp" select="@AirTemp"/>
            <xsl:with-param name="units" select="$units"/>
          </xsl:call-template>
        </xsl:attribute>

        <xsl:attribute name="water">
          <xsl:call-template name="tempConvert">
            <xsl:with-param name="temp" select="@WaterTempMD"/>
            <xsl:with-param name="units" select="$units"/>
          </xsl:call-template>
        </xsl:attribute>
      </temperature>

      <xsl:for-each select="profile">
        <sample>
          <xsl:attribute name="time">
            <xsl:call-template name="timeConvert">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="@sample * $delta"/>
              </xsl:with-param>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="depth">
            <xsl:call-template name="depthConvert">
              <xsl:with-param name="depth">
                <xsl:value-of select="@depth"/>
              </xsl:with-param>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
        </sample>
      </xsl:for-each>
      </divecomputer>

    </dive>
  </xsl:template>

  <!-- convert pressure to bars -->
  <xsl:template name="pressureConvert">
    <xsl:param name="number"/>
    <xsl:param name="units"/>

    <xsl:choose>
      <xsl:when test="$units != 'Metric'">
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
      <xsl:when test="$units != 'Metric'">
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
      <xsl:when test="$units != 'Metric'">
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
      <xsl:when test="$units != 'Metric'">
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
          <xsl:when test="$units != 'Metric'">
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
