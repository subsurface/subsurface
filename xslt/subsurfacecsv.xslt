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
    <divelog program="subsurface-import" version="2">
      <dives>
        <xsl:call-template name="printLine">
          <xsl:with-param name="line" select="substring-before(substring-after(//SubsurfaceCSV, $lf), $lf)"/>
          <xsl:with-param name="remaining" select="substring-after(substring-after(//SubsurfaceCSV, $lf), $lf)"/>
        </xsl:call-template>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template name="printLine">
    <xsl:param name="line"/>
    <xsl:param name="remaining"/>

    <xsl:call-template name="printFields">
      <xsl:with-param name="line" select="$line"/>
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
        <xsl:call-template name="getFieldByIndex">
          <xsl:with-param name="index" select="0"/>
          <xsl:with-param name="line" select="$line"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:attribute name="duration">
        <xsl:call-template name="getFieldByIndex">
          <xsl:with-param name="index" select="3"/>
          <xsl:with-param name="line" select="$line"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:attribute name="tags">
        <xsl:call-template name="getFieldByIndex">
          <xsl:with-param name="index" select="22"/>
          <xsl:with-param name="line" select="$line"/>
        </xsl:call-template>
      </xsl:attribute>

      <xsl:variable name="rating">
        <xsl:call-template name="getFieldByIndex">
          <xsl:with-param name="index" select="18"/>
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
          <xsl:with-param name="index" select="19"/>
          <xsl:with-param name="line" select="$line"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:if test="$visibility != ''">
        <xsl:attribute name="visibility">
          <xsl:value-of select="$visibility"/>
        </xsl:attribute>
      </xsl:if>

      <divecomputerid deviceid="ffffffff" model="csv" />

      <depth>
        <xsl:variable name="max">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="4"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="mean">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="5"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$max != ''">
          <xsl:attribute name="max">
            <xsl:value-of select="$max"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$mean != ''">
          <xsl:attribute name="mean">
            <xsl:value-of select="$mean"/>
          </xsl:attribute>
        </xsl:if>
      </depth>

      <divetemperature>
        <xsl:variable name="air">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="6"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="water">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="7"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$air != ''">
          <xsl:attribute name="air">
            <xsl:value-of select="$air"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$water != ''">
          <xsl:attribute name="water">
            <xsl:value-of select="$water"/>
          </xsl:attribute>
        </xsl:if>
      </divetemperature>

      <cylinder>
        <xsl:variable name="size">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="8"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="start">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="9"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="end">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="10"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="o2">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="11"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="he">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="12"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$size != ''">
          <xsl:attribute name="size">
            <xsl:value-of select="$size"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$start != ''">
          <xsl:attribute name="start">
            <xsl:value-of select="$start"/>
          </xsl:attribute>
        </xsl:if>
        <xsl:if test="$end != ''">
          <xsl:attribute name="end">
            <xsl:value-of select="$end"/>
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
      </cylinder>

      <location>
        <xsl:variable name="gps">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="14"/>
            <xsl:with-param name="line" select="$line"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="location">
          <xsl:call-template name="getFieldByIndex">
            <xsl:with-param name="index" select="13"/>
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
          <xsl:with-param name="index" select="15"/>
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
          <xsl:with-param name="index" select="16"/>
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
          <xsl:with-param name="index" select="17"/>
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
          <xsl:with-param name="index" select="20"/>
          <xsl:with-param name="line" select="$line"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:if test="$notes != ''">
        <notes>
          <xsl:value-of select="$notes"/>
        </notes>
      </xsl:if>

      <xsl:variable name="weight">
        <xsl:call-template name="getFieldByIndex">
          <xsl:with-param name="index" select="21"/>
          <xsl:with-param name="line" select="$line"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:if test="$weight != ''">
        <weightsystem description="unknown">
          <xsl:attribute name="weight">
            <xsl:value-of select="$weight"/>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

    </dive>
  </xsl:template>

  <xsl:template name="getFieldByIndex">
    <xsl:param name="index"/>
    <xsl:param name="line"/>
    <xsl:choose>
      <xsl:when test="$index > 0">
        <xsl:choose>
          <xsl:when test="substring($line, 1, 1) = '&quot;'">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="$index -1"/>
              <xsl:with-param name="line" select="substring-after($line, $fs)"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="$index -1"/>
              <xsl:with-param name="line" select="substring-after($line, $fs)"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="substring($line, 1, 1) = '&quot;'">
            <xsl:choose>
              <xsl:when test="substring-before($line,'&quot;$fs') != ''">
                <xsl:value-of select="substring-before($line,'&quot;$fs')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:if test="substring-after($line, '&quot;$fs') = ''">
                  <xsl:value-of select="substring-before(substring-after($line, '&quot;'), '&quot;')"/>
                </xsl:if>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:when>

          <xsl:otherwise>
            <xsl:choose>
              <xsl:when test="substring-before($line,$fs) != ''">
                <xsl:value-of select="substring-before($line,$fs)"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:if test="substring-after($line, $fs) = ''">
                  <xsl:value-of select="$line"/>
                </xsl:if>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>

  </xsl:template>
</xsl:stylesheet>
