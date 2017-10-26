<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <dives>
      <program name="subsurface-import" version="1"/>
      <xsl:apply-templates select="/JDiveLog/JDive"/>
    </dives>
  </xsl:template>

  <xsl:template match="JDive">
    <xsl:variable name="units" select="UNITS"/>

    <dive number="{DiveNum}">
      <xsl:attribute name="date">
        <xsl:value-of select="concat(DATE/YEAR,'-',format-number(DATE/MONTH,
           '00'), '-', format-number(DATE/DAY, '00'))"/>
      </xsl:attribute>

      <xsl:attribute name="time">
        <xsl:value-of select="concat(format-number(TIME/HOUR, '00'), ':',
           format-number(TIME/MINUTE, '00'))"/>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:call-template name="timeConvert">
          <xsl:with-param name="timeSec" select="Duration"/>
          <xsl:with-param name="units" select="$units"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:choose>
        <xsl:when test="Average_Depth != ''">
          <depth max="{concat(Depth,' m')}" mean="{concat(Average_Depth, ' m')}"/>
        </xsl:when>
        <xsl:otherwise>
          <depth max="{concat(Depth,' m')}"/>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:if test="TEMPERATURE != ''">
        <xsl:choose>
          <xsl:when test="$units = 'si'">
            <temperature water="{concat(format-number(TEMPERATURE - 273.15, '0.0'), ' C')}"/>
          </xsl:when>
          <xsl:otherwise>
            <temperature water="{concat(TEMPERATURE, ' C')}"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>

      <xsl:if test="diveSiteId != ''">
        <xsl:variable name="diveSite" select="diveSiteId"/>
        <location>
          <xsl:value-of select="concat(/JDiveLog/Masterdata/DiveSites/DiveSite[privateId=$diveSite]/country, ' ', /JDiveLog/Masterdata/DiveSites/DiveSite[privateId=$diveSite]/state, ' ', /JDiveLog/Masterdata/DiveSites/DiveSite[privateId=$diveSite]/city, ' ', /JDiveLog/Masterdata/DiveSites/DiveSite[privateId=$diveSite]/spot)"/>
        </location>
      </xsl:if>

      <xsl:if test="Buddy">
        <buddy>
          <xsl:value-of select="Buddy"/>
        </buddy>
      </xsl:if>

      <xsl:if test="Equipment/Suit != ''">
	<suit>
          <xsl:value-of select="Equipment/Suit"/>
	</suit>
      </xsl:if>

      <xsl:if test="Equipment/Weight != ''">
        <weightsystem>
          <xsl:attribute name="weight">
            <xsl:choose>
              <xsl:when test="Equipment/Weight = 'none'">
                <xsl:value-of select="0" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="translate(Equipment/Weight, ',', '.')"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

      <notes>
        <xsl:if test="DiveActivity != ''">
Diveactivity: <xsl:value-of select="DiveActivity"/>
        </xsl:if>
        <xsl:if test="DiveType != ''">
Divetype: <xsl:value-of select="DiveType"/>
        </xsl:if>
        <xsl:if test="Equipment/Visibility != ''">
Visibility: <xsl:value-of select="Equipment/Visibility"/>
        </xsl:if>
        <xsl:if test="Equipment/Gloves != ''">
Gloves: <xsl:value-of select="Equipment/Gloves"/>
        </xsl:if>
        <xsl:if test="Comment != ''">
Comment: <xsl:value-of select="Comment"/>
        </xsl:if>
      </notes>

      <!-- cylinder -->
      <xsl:for-each select="Equipment/Tanks/Tank">
        <cylinder>
          <xsl:attribute name="o2">
            <xsl:choose>
              <xsl:when test="MIX/O2 != ''">
                <xsl:value-of select="concat(MIX/O2*100, '%')"/>
              </xsl:when>
              <xsl:otherwise>21.0%</xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
          <xsl:if test="MIX/HE != '0.0'">
            <xsl:attribute name="he">
              <xsl:value-of select="concat(MIX/HE*100, '%')"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:attribute name="size">
            <xsl:choose>
              <xsl:when test="MIX/TANK/TANKVOLUME != ''">
                <xsl:value-of select="concat(MIX/TANK/TANKVOLUME * 1000, ' l')"/>
              </xsl:when>
              <xsl:otherwise>0 l</xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
          <xsl:attribute name="start">
            <xsl:variable name="number" select="MIX/TANK/PSTART"/>
            <xsl:call-template name="pressure">
              <xsl:with-param name="number" select="$number"/>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:attribute name="end">
            <xsl:variable name="number" select="MIX/TANK/PEND"/>
            <xsl:call-template name="pressure">
              <xsl:with-param name="number" select="$number"/>
              <xsl:with-param name="units" select="$units"/>
            </xsl:call-template>
          </xsl:attribute>
        </cylinder>
      </xsl:for-each>
      <!-- end cylinder -->

      <!-- DELTA is the sample interval -->
      <xsl:variable name="delta">
        <xsl:choose>
          <xsl:when test="DIVE/SAMPLES/DELTA != ''">
            <xsl:choose>
              <xsl:when test="$units = 'si'">
                <xsl:value-of select="DIVE/SAMPLES/DELTA"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="round(60 * DIVE/SAMPLES/DELTA)"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>
          <xsl:otherwise>0</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <!-- end delta -->

      <!-- events -->
      <xsl:for-each select="DIVE/SAMPLES/ALARM">
        <xsl:if test=". != 'SURFACE'">
	  <event type="">
            <xsl:attribute name="name">
              <xsl:choose>
                <xsl:when test=". = 'SLOW'">
                  <xsl:value-of select="'ascent'"/>
                </xsl:when>
                <xsl:when test=". = 'ATTENTION'">
                  <xsl:value-of select="'violation'"/>
                </xsl:when>
                <xsl:when test=". = 'DECO'">
                  <xsl:value-of select="'deco'"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="."/>
                </xsl:otherwise>
              </xsl:choose>
	    </xsl:attribute>
            <xsl:attribute name="time">
              <xsl:choose>
                <xsl:when test="$delta != '0'">
                  <xsl:call-template name="timeConvert">
                    <xsl:with-param name="timeSec" select="count(preceding-sibling::D) * $delta"/>
                    <xsl:with-param name="units" select="'si'"/>
                  </xsl:call-template>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:call-template name="timeConvert">
                  <xsl:with-param name="timeSec" select="preceding-sibling::T[1]"/>
                    <xsl:with-param name="units" select="$units"/>
                  </xsl:call-template>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </event>
        </xsl:if>
      </xsl:for-each>
      <!-- end events -->

      <!-- gas change -->
      <xsl:for-each select="DIVE/SAMPLES/SWITCH">
        <event name="gaschange">
          <xsl:attribute name="time">
            <xsl:choose>
              <xsl:when test="$delta != '0'">
                <xsl:call-template name="timeConvert">
                  <xsl:with-param name="timeSec" select="count(preceding-sibling::D) * $delta"/>
                  <xsl:with-param name="units" select="'si'"/>
                </xsl:call-template>
              </xsl:when>
              <xsl:otherwise>
                <xsl:call-template name="timeConvert">
                  <xsl:with-param name="timeSec">
                    <xsl:choose>
                      <xsl:when test="preceding-sibling::T[1] != ''">
                        <xsl:value-of select="preceding-sibling::T[1]"/>
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:value-of select="'0'"/>
                      </xsl:otherwise>
                    </xsl:choose>
                  </xsl:with-param>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
          <xsl:attribute name="o2">
            <xsl:value-of select="ancestor::DIVE/GASES/MIX[MIXNAME=current()]/O2 * 100" />
          </xsl:attribute>
          <xsl:attribute name="he">
            <xsl:value-of select="ancestor::DIVE/GASES/MIX[MIXNAME=current()]/HE * 100" />
          </xsl:attribute>
        </event>
      </xsl:for-each>
      <!-- end gas change -->

      <!-- dive sample - all the depth and temp readings -->
      <xsl:for-each select="DIVE/SAMPLES/D">
        <sample>
          <xsl:choose>
            <xsl:when test="$delta != '0'">
              <xsl:variable name="timeSec" select="(position() - 1) * $delta"/>
              <xsl:attribute name="time">
                <xsl:value-of select="concat(floor($timeSec div 60), ':',
                  format-number(floor($timeSec mod 60), '00'), ' min')"/>
              </xsl:attribute>
            </xsl:when>
            <xsl:otherwise>
              <xsl:attribute name="time">
                <xsl:call-template name="timeConvert">
                  <xsl:with-param name="timeSec" select="preceding-sibling::T[1]"/>
                  <xsl:with-param name="units" select="$units"/>
                </xsl:call-template>
              </xsl:attribute>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:attribute name="depth">
            <xsl:value-of select="concat(., ' m')"/>
          </xsl:attribute>
          <xsl:if test="name(following-sibling::*[1]) = 'TEMPERATURE'">
            <xsl:attribute name="temp">
              <xsl:choose>
                <xsl:when test="$units = 'si'">
                  <xsl:value-of select="concat(format-number(following-sibling::TEMPERATURE - 273.15, '0.0'), ' C')"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="concat(following-sibling::TEMPERATURE, ' C')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="(name(following-sibling::*[1]) = 'DECOINFO') or (name(following-sibling::*[2]) = 'DECOINFO')">
            <xsl:choose>
              <xsl:when test="following-sibling::DECOINFO[1] = '0.0'">
                <xsl:attribute name="ndl">
                  <xsl:call-template name="timeConvertMin">
                    <xsl:with-param name="timeMin" select="following-sibling::DECOINFO[1]/@tfs"/>
                    <xsl:with-param name="units" select="$units"/>
                  </xsl:call-template>
                </xsl:attribute>
              </xsl:when>
              <xsl:otherwise>
                <xsl:attribute name="stoptime">
                  <xsl:call-template name="timeConvertMin">
                    <xsl:with-param name="timeMin" select="following-sibling::DECOINFO[1]/@tfs"/>
                    <xsl:with-param name="units" select="$units"/>
                  </xsl:call-template>
                </xsl:attribute>
                <xsl:attribute name="stopdepth">
                  <xsl:value-of select="concat(following-sibling::DECOINFO[1], ' m')"/>
                </xsl:attribute>
		<xsl:attribute name="ndl">
			<xsl:value-of select="'0:00 min'"/>
		</xsl:attribute>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:if>
        </sample>
      </xsl:for-each>
      <!-- dive sample -->

    </dive>
  </xsl:template>
  <!-- end JDive -->

  <!-- convert pressure to bars -->
  <xsl:template name="pressure">
    <xsl:param name="number"/>
    <xsl:param name="units"/>

    <xsl:variable name="pressure">
      <xsl:choose>
        <xsl:when test="$number != ''">
          <xsl:variable name="Exp" select="substring-after($number, 'E')"/>
          <xsl:variable name="Man" select="substring-before($number, 'E')"/>
          <xsl:variable name="Fac" select="substring('100000000000000000000', 1, $Exp + 1)"/>
          <xsl:choose>
            <xsl:when test="$Exp != ''">
              <xsl:value-of select="(number($Man) * number($Fac))"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$number"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>0</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="$units = 'si'">
        <xsl:value-of select="concat(($pressure div 100000), ' bar')"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($pressure, ' bar')"/>
      </xsl:otherwise>
    </xsl:choose>

  </xsl:template>
  <!-- end convert pressure -->

  <!-- convert time in seconds to minutes:seconds -->
  <xsl:template name="timeConvert">
    <xsl:param name="timeSec"/>
    <xsl:param name="units"/>

    <xsl:if test="$timeSec != ''">
      <xsl:choose>
        <xsl:when test="$units = 'si'">
          <xsl:value-of select="concat(floor(number($timeSec) div 60), ':',    format-number(floor(number($timeSec) mod 60), '00'), ' min')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="substring-after($timeSec, '.') &gt;= 60 or string-length(substring-after($timeSec, '.')) &lt; 2">
              <xsl:value-of select="concat(substring-before($timeSec, '.'), ':', format-number(round(substring-after(format-number($timeSec, '.00'), '.') * .6), '00'), ' min')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat(substring-before($timeSec, '.'), ':', format-number(substring-after($timeSec, '.'), '00'), ' min')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>
  <!-- end convert time -->

  <!-- convert time in minutes.decimal to minutes:seconds -->
  <xsl:template name="timeConvertMin">
    <xsl:param name="timeMin"/>
    <xsl:param name="units"/>

    <xsl:if test="$timeMin != ''">
      <xsl:choose>
        <xsl:when test="$units = 'si'">
          <xsl:value-of select="concat(substring-before($timeMin, '.'), ':',    format-number(substring-after($timeMin, '.'), '00'), ' min')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="substring-after($timeMin, '.') &gt;= 60 or string-length(substring-after($timeMin, '.')) &lt; 2">
              <xsl:value-of select="concat(substring-before($timeMin, '.'), ':', format-number(round(substring-after(format-number($timeMin, '.00'), '.') * .6), '00'), ' min')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat(substring-before($timeMin, '.'), ':', format-number(substring-after($timeMin, '.'), '00'), ' min')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>
  <!-- end convert time -->
</xsl:stylesheet>
