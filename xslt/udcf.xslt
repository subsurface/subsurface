<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
    <divelog program="subsurface-import" version="2">
      <settings>
        <divecomputerid deviceid="ffffffff">
          <xsl:apply-templates select="/PROFILE/DEVICE|/profile/device"/>
        </divecomputerid>
      </settings>
      <dives>
	      <xsl:apply-templates select="/PROFILE/REPGROUP/DIVE|/profile/repgroup/dive"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="DEVICE|device">
    <xsl:if test="MODEL|model != ''">
      <xsl:attribute name="model">
        <xsl:value-of select="MODEL|model"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="version|VERSION != ''">
      <xsl:attribute name="serial">
        <xsl:value-of select="VERSION|version"/>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

  <xsl:template match="DIVE|dive">
    <dive>
      <xsl:attribute name="date">
        <xsl:for-each select="DATE/YEAR|DATE/MONTH|DATE/DAY|date/year|date/month|date/day">
          <xsl:if test="position() != 1">-</xsl:if>
          <xsl:value-of select="."/>
        </xsl:for-each>
      </xsl:attribute>

      <xsl:attribute name="time">
        <xsl:for-each select="TIME/HOUR|TIME/MINUTE|time/hour|time/minute">
          <xsl:if test="position() != 1">:</xsl:if>
          <xsl:value-of select="."/>
        </xsl:for-each>
      </xsl:attribute>

      <location>
        <xsl:value-of select="PLACE|place"/>
      </location>

      <xsl:if test="TEMPERATURE|temperature != ''">
        <temperature>
          <xsl:attribute name="water">
            <xsl:value-of select="concat(TEMPERATURE|temperature, ' C')"/>
          </xsl:attribute>
        </temperature>
      </xsl:if>

      <divecomputer deviceid="ffffffff">
        <xsl:attribute name="model">
          <xsl:value-of select="/PROFILE/DEVICE/MODEL|/profile/device/model"/>
        </xsl:attribute>
      </divecomputer>

      <xsl:for-each select="GASES/MIX|gases/mix">
        <cylinder>
          <xsl:attribute name="description">
            <xsl:value-of select="MIXNAME|mixname"/>
          </xsl:attribute>
          <xsl:attribute name="size">
            <xsl:value-of select="concat(TANK/TANKVOLUME|tank/tankvolume, ' l')"/>
          </xsl:attribute>
          <xsl:attribute name="start">
            <xsl:value-of select="TANK/PSTART|tank/pstart"/>
          </xsl:attribute>
          <xsl:attribute name="end">
            <xsl:value-of select="TANK/PEND|tank/pend"/>
          </xsl:attribute>
          <xsl:attribute name="o2">
            <xsl:value-of select="O2|o2"/>
          </xsl:attribute>
          <xsl:attribute name="he">
            <xsl:value-of select="HE|he"/>
          </xsl:attribute>
        </cylinder>
      </xsl:for-each>

      <xsl:choose>

	<!-- samples recorded at irregular internal, but storing time stamp -->
	<xsl:when test="timedepthmode">
	  <!-- gas change -->
	  <xsl:for-each select="SAMPLES/SWITCH|samples/switch">
	    <event name="gaschange">
	      <xsl:variable name="timeSec" select="following-sibling::T|following-sibling::t"/>
	      <xsl:attribute name="time">
		<xsl:value-of select="concat(floor($timeSec div 60), ':',
		  format-number(floor($timeSec mod 60), '00'), ' min')"/>
	      </xsl:attribute>
	      <xsl:attribute name="value">
		<xsl:value-of select="ancestor::DIVE/GASES/MIX[MIXNAME=current()]/O2|ancestor::dive/gases/mix[mixname=current()]/o2 * 100" />
	      </xsl:attribute>
	    </event>
	  </xsl:for-each>
	  <!-- end gas change -->

	  <!-- samples -->
	  <xsl:for-each select="SAMPLES/D|samples/d">
	    <sample>
	      <xsl:variable name="timeSec" select="preceding-sibling::T[position()=1]|preceding-sibling::t[position()=1]"/>
	      <xsl:attribute name="time">
		<xsl:value-of select="concat(floor($timeSec div 60), ':',
		  format-number(floor($timeSec mod 60), '00'), ' min')"/>
	      </xsl:attribute>
	      <xsl:attribute name="depth">
		<xsl:value-of select="concat(., ' m')"/>
	      </xsl:attribute>
	    </sample>
	  </xsl:for-each>
	  <!-- end samples -->
	</xsl:when>

	<!-- sample recorded at even internals -->
	<xsl:otherwise>
	  <xsl:variable name="delta" select="SAMPLES/DELTA|samples/delta"/>

	  <!-- gas change -->
	  <xsl:for-each select="SAMPLES/SWITCH|samples/switch">
	    <event name="gaschange">
	      <xsl:variable name="timeSec" select="count(preceding-sibling::D|preceding-sibling::d) * $delta"/>
	      <xsl:attribute name="time">
		<xsl:value-of select="concat(floor($timeSec div 60), ':',
		  format-number(floor($timeSec mod 60), '00'), ' min')"/>
	      </xsl:attribute>
	      <xsl:attribute name="value">
		<xsl:value-of select="ancestor::DIVE/GASES/MIX[MIXNAME=current()]/O2|ancestor::dive/gases/mix[mixname=current()]/o2 * 100" />
	      </xsl:attribute>
	    </event>

	  </xsl:for-each>
	  <!-- end gas change -->

	  <!-- samples -->
	  <xsl:for-each select="SAMPLES/D|samples/d">
	    <sample>
	      <xsl:variable name="timeSec" select="(position() - 1) * $delta"/>
	      <xsl:attribute name="time">
		<xsl:value-of select="concat(floor($timeSec div 60), ':',
		  format-number(floor($timeSec mod 60), '00'), ' min')"/>
	      </xsl:attribute>
	      <xsl:attribute name="depth">
		<xsl:value-of select="concat(., ' m')"/>
	      </xsl:attribute>
	    </sample>
	  </xsl:for-each>
	  <!-- end samples -->

	</xsl:otherwise>
      </xsl:choose>
    </dive>
  </xsl:template>
</xsl:stylesheet>
