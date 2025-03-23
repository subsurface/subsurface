<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:param name="separatorIndex" select="separatorIndex"/>
  <xsl:param name="units" select="units"/>
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
    <xsl:variable name="cylinders">
      <xsl:call-template name="countCylinders">
        <xsl:with-param name="line" select="substring-before(//SubsurfaceCSV, $lf)"/>
        <xsl:with-param name="count" select="'0'"/>
        <xsl:with-param name="index" select="'10'"/>
      </xsl:call-template>
    </xsl:variable>
    <divelog program="subsurface-import" version="2">
      <dives>
        <xsl:call-template name="printLine">
          <xsl:with-param name="line" select="substring-before(substring-after(//SubsurfaceCSV, $lf), $lf)"/>
          <xsl:with-param name="remaining" select="substring-after(substring-after(//SubsurfaceCSV, $lf), $lf)"/>
          <xsl:with-param name="cylinders" select="$cylinders"/>
        </xsl:call-template>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template name="printLine">
    <xsl:param name="line"/>
    <xsl:param name="remaining"/>
    <xsl:param name="cylinders"/>

    <xsl:call-template name="printFields">
      <xsl:with-param name="line" select="$line"/>
      <xsl:with-param name="remaining" select="$remaining"/>
      <xsl:with-param name="cylinders" select="$cylinders"/>
    </xsl:call-template>

    <xsl:if test="$remaining != ''">
      <xsl:call-template name="printLine">
        <xsl:with-param name="line" select="substring-before($remaining, $lf)"/>
        <xsl:with-param name="remaining" select="substring-after($remaining, $lf)"/>
        <xsl:with-param name="cylinders" select="$cylinders"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="printFields">
    <xsl:param name="line"/>
    <xsl:param name="remaining"/>
    <xsl:param name="cylinders"/>

    <xsl:variable name="number">
      <xsl:call-template name="getFieldByIndex">
        <xsl:with-param name="index" select="0"/>
        <xsl:with-param name="line" select="$line"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="$number >= 0">
      <dive>
        <xsl:attribute name="date">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="1"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:attribute>

        <xsl:attribute name="time">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="2"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:attribute>

        <xsl:attribute name="number">
          <xsl:value-of select="$number"/>
        </xsl:attribute>

        <xsl:attribute name="duration">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="3"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:attribute>

        <!-- If reading of SAC is added at some point, it is already available in our own CSV import -->
        <xsl:variable name="sac">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="4"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:attribute name="sac">
          <xsl:choose>
            <xsl:when test="$units = 1">
              <xsl:value-of select="format-number($sac * 0.035315, '#.##')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$sac"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>

        <xsl:attribute name="tags">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="24 + ($cylinders - 1) * 5"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:attribute>

        <xsl:variable name="rating">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="20 + ($cylinders - 1) * 5"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$rating != ''">
          <xsl:attribute name="rating">
            <xsl:value-of select="$rating"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:variable name="visibility">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="21 + ($cylinders - 1) * 5"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$visibility != ''">
          <xsl:attribute name="visibility">
            <xsl:value-of select="$visibility"/>
          </xsl:attribute>
        </xsl:if>

        <xsl:variable name="divemode">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="7"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
	</xsl:variable>
        <divecomputer deviceid="ffffffff" model="SubsurfaceCSV">
          <xsl:attribute name="dctype">
            <xsl:value-of select="$divemode"/>
          </xsl:attribute>
        </divecomputer>


        <depth>
          <xsl:variable name="max">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="5"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="mean">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="6"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:if test="$max != ''">
            <xsl:attribute name="max">
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$max"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="concat(round(translate(translate($max, translate($max, '0123456789,.', ''), ''), ',', '.') * 0.3048 * 1000) div 1000, ' m')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$mean != ''">
            <xsl:attribute name="mean">
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$mean"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="concat(round(translate(translate($mean, translate($mean, '0123456789,.', ''), ''), ',', '.') * 0.3048 * 1000) div 1000, ' m')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
        </depth>

        <divetemperature>
          <xsl:variable name="air">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="8"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="water">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="9"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:if test="$air != ''">
            <xsl:attribute name="air">
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$air"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="concat(format-number((translate(translate($air, translate($air, '0123456789,.', ''), ''), ',', '.') - 32) * 5 div 9, '0.0'), ' C')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$water != ''">
            <xsl:attribute name="water">
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$water"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="concat(format-number((translate(translate($water, translate($water, '0123456789,.', ''), ''), ',', '.') - 32) * 5 div 9, '0.0'), ' C')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </xsl:if>
        </divetemperature>

        <!-- CYLINDER -->
        <xsl:call-template name="parseCylinders">
          <xsl:with-param name="line" select="$line"/>
          <xsl:with-param name="cylinders" select="$cylinders"/>
          <xsl:with-param name="count" select="'0'"/>
          <xsl:with-param name="index" select="'10'"/>
          <xsl:with-param name="divemode" select="$divemode"/>
        </xsl:call-template>

        <location>
          <xsl:variable name="gps">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="16 + ($cylinders - 1) * 5"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="location">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="15 + ($cylinders - 1) * 5"/>
              <xsl:with-param name="line" select="$line"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:if test="$gps != ''">
            <xsl:attribute name="gps">
              <xsl:value-of select="$gps"/>
            </xsl:attribute>
          </xsl:if>
          <xsl:if test="$location != ''">
            <xsl:value-of select="$location"/>
          </xsl:if>
        </location>

        <xsl:variable name="dm">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="17 + ($cylinders - 1) * 5"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$dm != ''">
          <divemaster>
            <xsl:value-of select="$dm"/>
          </divemaster>
        </xsl:if>

        <xsl:variable name="buddy">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="18 + ($cylinders - 1) * 5"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$buddy != ''">
          <buddy>
            <xsl:value-of select="$buddy"/>
          </buddy>
        </xsl:if>

        <xsl:variable name="suit">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="19 + ($cylinders - 1) * 5"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$suit != ''">
          <suit>
            <xsl:value-of select="$suit"/>
          </suit>
        </xsl:if>

        <xsl:variable name="notes">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="22 + ($cylinders - 1) * 5"/>
            <xsl:with-param name="line" select="$line"/>
            <xsl:with-param name="remaining" select="$remaining"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$notes != ''">
          <notes>
            <xsl:value-of select="$notes"/>
          </notes>
        </xsl:if>

        <xsl:variable name="weight">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="23 + ($cylinders - 1) * 5"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$weight != ''">
          <weightsystem description="unknown">
            <xsl:attribute name="weight">
              <xsl:choose>
                <xsl:when test="$units = 0">
                  <xsl:value-of select="$weight"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="concat(format-number((translate($weight, translate($weight, '0123456789', ''), '') * 0.453592), '#.##'), ' kg')"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
          </weightsystem>
        </xsl:if>

      </dive>
    </xsl:if>
  </xsl:template>

  <xsl:template name="countCylinders">
    <xsl:param name="line"/>
    <xsl:param name="count"/>
    <xsl:param name="index"/>

    <xsl:variable name="field">
      <xsl:call-template name="getFieldByIndex">
        <xsl:with-param name="index" select="$index"/>
        <xsl:with-param name="line" select="$line"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="substring-before($field, ' ') = 'cylinder'">
        <xsl:call-template name="countCylinders">
          <xsl:with-param name="line" select="$line"/>
          <xsl:with-param name="count" select="$count + 1"/>
          <xsl:with-param name="index" select="$index + 5"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$count"/>
      </xsl:otherwise>
    </xsl:choose>

  </xsl:template>


  <xsl:template name="parseCylinders">
    <xsl:param name="line"/>
    <xsl:param name="cylinders"/>
    <xsl:param name="count"/>
    <xsl:param name="index"/>
    <xsl:param name="divemode"/>

    <xsl:variable name="field">
      <xsl:call-template name="getFieldByIndex">
        <xsl:with-param name="index" select="$index"/>
        <xsl:with-param name="line" select="$line"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:if test="$count &lt; $cylinders">
      <cylinder>
        <xsl:variable name="size">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$index"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="start">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$index + 1"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="end">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$index + 2"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="o2">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$index + 3"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="he">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="$index + 4"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:if test="$size != ''">
          <xsl:attribute name="size">
            <xsl:choose>
              <xsl:when test="substring($size, 1, 2) = 'AL'">
                <xsl:value-of select="format-number((translate($size, translate($size, '0123456789', ''), '') * 14.7 div 3000) div 0.035315, '#.#')"/>
              </xsl:when>
              <xsl:when test="substring($size, 1, 2) = 'LP'">
                <xsl:value-of select="format-number((translate($size, translate($size, '0123456789', ''), '') * 14.7 div 2400) div 0.035315, '#.#')"/>
              </xsl:when>
              <xsl:when test="substring($size, 1, 2) = 'HP'">
                <xsl:value-of select="format-number((translate($size, translate($size, '0123456789', ''), '') * 14.7 div 3440) div 0.035315, '#.#')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:choose>
                  <xsl:when test="$units = 0">
                    <xsl:value-of select="$size"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="format-number((translate($size, translate($size, '0123456789,.', ''), '') * 14.7 div 3000) div 0.035315, '#.#')"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="substring($size, 1, 2) = 'AL' or substring($size, 1, 2) = 'LP' or substring($size, 1, 2) = 'HP'">
          <xsl:attribute name="description">
            <xsl:value-of select="$size"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$start != ''">
          <xsl:attribute name="start">
            <xsl:choose>
              <xsl:when test="$units = 0">
                <xsl:value-of select="$start"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="concat(format-number((translate($start, translate($start, '0123456789', ''), '') div 14.5037738007), '#'), ' bar')"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$end != ''">
          <xsl:attribute name="end">
            <xsl:choose>
              <xsl:when test="$units = 0">
                <xsl:value-of select="$end"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="concat(format-number((translate($end, translate($end, '0123456789', ''), '') div 14.5037738007), '#'), ' bar')"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$o2 != ''">
          <xsl:attribute name="o2">
            <xsl:value-of select="$o2"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$he != ''">
          <xsl:attribute name="he">
            <xsl:value-of select="$he"/>
          </xsl:attribute>
        </xsl:if>

	<xsl:if test="$divemode = 'CCR' and number($o2) >= 21">
          <xsl:attribute name="use">
            diluent
          </xsl:attribute>
        </xsl:if>
      </cylinder>

      <xsl:call-template name="parseCylinders">
        <xsl:with-param name="line" select="$line"/>
        <xsl:with-param name="cylinders" select="$cylinders"/>
        <xsl:with-param name="count" select="$count + 1"/>
        <xsl:with-param name="index" select="$index + 5"/>
        <xsl:with-param name="divemode" select="$divemode"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
