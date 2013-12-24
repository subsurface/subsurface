<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:u="http://schemas.datacontract.org/2004/07/Suunto.Diving.Dal"
  version="1.0">
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <divelog program="subsurface-import" version="2">
      <settings>
        <divecomputerid deviceid="ffffffff">
          <xsl:attribute name="model">
            <xsl:value-of select="/u:Dive/u:Source"/>
          </xsl:attribute>
          <xsl:attribute name="serial">
            <xsl:value-of select="/u:Dive/u:SourceSerialNumber"/>
          </xsl:attribute>
        </divecomputerid>
      </settings>
      <dives>
        <dive>
          <xsl:attribute name="duration">
            <xsl:call-template name="sec2time">
              <xsl:with-param name="timeSec">
                <xsl:value-of select="/u:Dive/u:DiveTime"/>
              </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>
          <xsl:call-template name="datetime">
            <xsl:with-param name="value">
              <xsl:value-of select="/u:Dive/u:StartTime"/>
            </xsl:with-param>
          </xsl:call-template>
          <xsl:if test="/u:Dive/u:Visibility != ''">
            <xsl:attribute name="visibility">
              <xsl:value-of select="/u:Dive/u:Visibility"/>
            </xsl:attribute>
          </xsl:if>
          <depth>
            <xsl:attribute name="max">
              <xsl:value-of select="/u:Dive/u:MaxDepth"/>
            </xsl:attribute>
            <xsl:attribute name="min">
              <xsl:value-of select="aou"/>
            </xsl:attribute>
          </depth>
          <xsl:for-each select="/u:Dive/u:Marks/u:Mark">
            <event>
              <xsl:attribute name="time">
                <xsl:call-template name="sec2time">
                  <xsl:with-param name="timeSec">
                    <xsl:value-of select="u:MarkTime"/>
                  </xsl:with-param>
                </xsl:call-template>
              </xsl:attribute>
              <xsl:attribute name="name">
                <xsl:choose>
                  <xsl:when test="u:Type = '19'">
                    <xsl:value-of select="'surface'"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="u:Type"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:attribute>
            </event>
          </xsl:for-each>
          <xsl:for-each select="/u:Dive/u:DiveMixtures/u:DiveMixture">
            <cylinder>
              <xsl:if test="u:Size &gt; 0">
                <xsl:attribute name="size">
                  <xsl:value-of select="u:Size"/>
                </xsl:attribute>
              </xsl:if>
              <xsl:if test="u:StartPressure &gt; 0">
                <xsl:attribute name="start">
                  <xsl:value-of select="u:StartPressure"/>
                </xsl:attribute>
                <xsl:attribute name="end">
                  <xsl:value-of select="u:EndPressure"/>
                </xsl:attribute>
              </xsl:if>
              <xsl:attribute name="o2">
                <xsl:value-of select="u:Oxygen"/>
              </xsl:attribute>
              <xsl:attribute name="he">
                <xsl:value-of select="u:Helium"/>
              </xsl:attribute>
            </cylinder>
          </xsl:for-each>
          <temperature>
            <xsl:attribute name="water">
              <xsl:value-of select="/u:Dive/u:BottomTemperature"/>
            </xsl:attribute>
            <xsl:attribute name="air">
              <xsl:value-of select="/u:Dive/u:StartTemperature"/>
            </xsl:attribute>
          </temperature>

          <notes>
            <xsl:value-of select="/u:Dive/u:Note"/>
          </notes>

          <sampleinterval>
            <xsl:value-of select="/u:Dive/u:SampleInterval"/>
          </sampleinterval>

          <xsl:if test="/u:Dive/u:Weight != ''">
            <weightsystem>
              <xsl:attribute name="weight">
                <xsl:value-of select="concat(/u:Dive/u:Weight, ' kg')"/>
              </xsl:attribute>
              <xsl:attribute name="description">
                <xsl:value-of select="'unknown'"/>
              </xsl:attribute>
            </weightsystem>
          </xsl:if>

          <blob>
            <xsl:attribute name="profileblob">
              <xsl:value-of select="/u:Dive/u:ProfileBlob"/>
            </xsl:attribute>
            <xsl:attribute name="temperatureblob">
              <xsl:value-of select="/u:Dive/u:TemperatureBlob"/>
            </xsl:attribute>
            <xsl:attribute name="pressureblob">
              <xsl:value-of select="/u:Dive/u:PressureBlob"/>
            </xsl:attribute>
          </blob>

        </dive>
      </dives>

    </divelog>
  </xsl:template>

</xsl:stylesheet>
