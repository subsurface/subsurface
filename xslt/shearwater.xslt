<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>
  <xsl:include href="commonTemplates.xsl"/>

  <xsl:key name="gases" match="diveLogRecord" use="concat(fractionO2, '/', fractionHe)" />

  <xsl:template match="/">
    <divelog program='subsurface-import' version='2'>
      <settings>
        <xsl:for-each select="/dive/diveLog">
          <divecomputerid deviceid="{computerSerial}">
            <xsl:attribute name="model">
              <xsl:value-of select="'Shearwater'" />
            </xsl:attribute>
            <xsl:attribute name="serial">
              <xsl:value-of select="computerSerial" />
            </xsl:attribute>
          </divecomputerid>
        </xsl:for-each>
      </settings>
      <dives>
        <xsl:apply-templates select="/dive/diveLog"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="diveLog">
    <xsl:variable name="units">
      <xsl:choose>
        <xsl:when test="imperialUnits = 'true'">
          <xsl:value-of select="'imperial'"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="'metric'"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <dive>
      <xsl:attribute name="number">
        <xsl:value-of select="number"/>
      </xsl:attribute>

      <xsl:variable name="datetime">
        <xsl:call-template name="convertDate">
          <xsl:with-param name="dateTime">
            <xsl:value-of select="startDate"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:variable>
      <xsl:attribute name="date">
        <xsl:value-of select="substring-before($datetime, ' ')"/>
      </xsl:attribute>
      <xsl:attribute name="time">
        <xsl:value-of select="substring-after($datetime, ' ')"/>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:value-of select="concat(maxTime, ' min')"/>
      </xsl:attribute>

      <xsl:variable name="timeMultiplier">
        <xsl:choose>
          <xsl:when test="diveLogRecords/diveLogRecord[1]/currentTime &gt; 100">
            <xsl:value-of select="'1000'"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="'1'"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <depth>
        <xsl:attribute name="max">
          <xsl:choose>
            <xsl:when test="$units = 'imperial'">
              <xsl:value-of select="maxDepth * 0.3048"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="maxDepth"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </depth>

      <surface>
        <xsl:attribute name="pressure">
          <xsl:value-of select="concat(startSurfacePressure div 1000, ' bar')"/>
        </xsl:attribute>
      </surface>

      <xsl:for-each select="diveLogRecords/diveLogRecord[generate-id() = generate-id(key('gases', concat(fractionO2, '/', fractionHe))[1])]">
        <xsl:if test="currentCircuitSetting = 1">
          <cylinder>
            <xsl:attribute name="description">
              <xsl:value-of select="concat(fractionO2 * 100, '/', fractionHe * 100)"/>
            </xsl:attribute>
            <xsl:attribute name="o2">
              <xsl:value-of select="concat(fractionO2 * 100, '%')"/>
            </xsl:attribute>
            <xsl:if test="fractionHe != 0">
              <xsl:attribute name="he">
                <xsl:value-of select="concat(fractionHe * 100, '%')"/>
              </xsl:attribute>
            </xsl:if>
          </cylinder>
        </xsl:if>
      </xsl:for-each>

      <divecomputer>
        <xsl:attribute name="deviceid">
          <xsl:value-of select="computerSerial"/>
        </xsl:attribute>

        <extradata key="startBatteryVoltage" value="{startBatteryVoltage}"/>
        <extradata key="endBatteryVoltage" value="{endBatteryVoltage}"/>
        <extradata key="computerFirmware" value="{computerFirmware}"/>
        <extradata key="computerSerial" value="{computerSerial}"/>
        <extradata key="computerSoftwareVersion" value="{computerSoftwareVersion}"/>
        <extradata key="computerModel" value="{computerModel}"/>
        <extradata key="logVersion" value="{logVersion}"/>
        <extradata key="product" value="{product}"/>
        <extradata key="features" value="{features}"/>
        <extradata key="decoModel" value="{decoModel}"/>
        <extradata key="vpmbConservatism" value="{vpmbConservatism}"/>
        <extradata key="gfMin" value="{gfMin}"/>
        <extradata key="gfMax" value="{gfMax}"/>

        <xsl:for-each select="diveLogRecords/diveLogRecord">
          <xsl:if test="(fractionO2 != preceding-sibling::diveLogRecord[1]/fractionO2) or (fractionHe != preceding-sibling::diveLogRecord[1]/fractionHe)">
            <event name="gaschange">
              <xsl:attribute name="time">
                <xsl:call-template name="sec2time">
                  <xsl:with-param name="timeSec">
                    <xsl:value-of select="currentTime div $timeMultiplier"/>
                  </xsl:with-param>
                </xsl:call-template>
              </xsl:attribute>
              <xsl:attribute name="o2">
                <xsl:value-of select="fractionO2"/>
              </xsl:attribute>
              <xsl:attribute name="he">
                <xsl:value-of select="fractionHe"/>
              </xsl:attribute>
            </event>
          </xsl:if>
        </xsl:for-each>

        <xsl:for-each select="diveLogRecords/diveLogRecord">
          <sample>
            <xsl:attribute name="time">
              <xsl:call-template name="sec2time">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="currentTime div $timeMultiplier"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>
            <xsl:attribute name="depth">
              <xsl:choose>
                <xsl:when test="$units = 'imperial'">
                  <xsl:value-of select="format-number(currentDepth * 0.3048, '0.00')"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="currentDepth"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
            <xsl:attribute name="temp">
              <xsl:choose>
                <xsl:when test="$units = 'imperial'">
                  <xsl:value-of select="concat(format-number((waterTemp - 32) * 5 div 9, '0.0'), ' C')"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="waterTemp"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
            <xsl:if test="currentCircuitSetting = 0">
              <xsl:attribute name="po2">
                <xsl:choose>
                  <xsl:when test="$units = 'imperial'">
                    <xsl:value-of select="concat(averagePPO2 div 14.5037738, ' bar')"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="concat(averagePPO2, ' bar')"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="tank0pressurePSI != '' and tank0pressurePSI &gt; 0 and tank0pressurePSI &lt; 4092">
              <xsl:attribute name="pressure0">
                <xsl:value-of select="concat(format-number((tank0pressurePSI * 2 div 14.5037738007), '#.##'), ' bar')"/>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="tank1pressurePSI != '' and tank1pressurePSI &gt; 0 and tank1pressurePSI &lt; 4092">
              <xsl:attribute name="pressure1">
                <xsl:value-of select="concat(format-number((tank1pressurePSI * 2 div 14.5037738007), '#.##'), ' bar')"/>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="currentNdl != '' and currentNdl != 0">
              <xsl:attribute name="ndl">
                <xsl:value-of select="concat(currentNdl, ':00 min')"/>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="firstStopDepth != ''">
              <xsl:attribute name="stopdepth">
                <xsl:choose>
                  <xsl:when test="$units = 'imperial'">
                    <xsl:value-of select="concat(format-number(firstStopDepth * 0.3048, '0.00'), ' m')"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="concat(firstStopDepth, ' m')"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:attribute>
              <xsl:attribute name="stoptime">
                <xsl:value-of select="concat(firstStopTime, ' min')"/>
              </xsl:attribute>
              <xsl:choose>
                <xsl:when test="firstStopDepth = 0">
                  <xsl:attribute name="in_deco">
                    <xsl:value-of select="'0'"/>
                  </xsl:attribute>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:attribute name="in_deco">
                    <xsl:value-of select="'1'"/>
                  </xsl:attribute>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:if>
          </sample>
        </xsl:for-each>
      </divecomputer>
    </dive>
  </xsl:template>
</xsl:stylesheet>
