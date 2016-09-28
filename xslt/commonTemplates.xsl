<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:u="http://www.streit.cc/uddf/3.2/"
  xmlns:u1="http://www.streit.cc/uddf/3.1/"
  >

  <!-- Convert ISO 8601 time format to "standard" date and time format
       -->
  <xsl:template name="datetime">
    <xsl:param name="value"/>

    <xsl:variable name="date">
      <xsl:choose>
        <xsl:when test="contains($value, 'T')">
          <xsl:value-of select="substring-before($value, 'T')"/>
        </xsl:when>
        <xsl:when test="contains($value, ' ')">
          <xsl:value-of select="substring-before($value, ' ')"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="time">
      <xsl:choose>
        <xsl:when test="contains($value, 'T')">
          <xsl:value-of select="substring-after($value, 'T')"/>
        </xsl:when>
        <xsl:when test="contains($value, ' ')">
          <xsl:value-of select="substring-after($value, ' ')"/>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:if test="$date != ''">
      <xsl:attribute name="date">
        <xsl:choose>
          <xsl:when test="contains($date, '-')">
            <xsl:value-of select="$date"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:variable name="year" select="substring($date, 1, 4)"/>
            <xsl:variable name="month" select="substring($date, 5, 2)"/>
            <xsl:variable name="day" select="substring($date, 7,2)"/>
            <xsl:value-of select="concat($year,'-',$month,'-',$day)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="$time != ''">
      <xsl:attribute name="time">
        <xsl:choose>
          <xsl:when test="contains($time, ':')">
            <xsl:value-of select="$time"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:variable name="hour" select="substring($time, 1, 2)"/>
            <xsl:variable name="minute" select="substring($time, 3, 2)"/>
            <xsl:variable name="second" select="substring($time, 5,2)"/>
            <xsl:value-of select="concat($hour,':',$minute,':',$second)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

  <!-- Convert units in Pascal given in scientific notation to normal
       decimal notation -->
  <xsl:template name="convertPascal">
    <xsl:param name="value"/>

    <xsl:variable name="number">
      <xsl:value-of select="translate($value, 'e', 'E')"/>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="contains($number, 'E')">
        <xsl:variable name="pressure">
          <xsl:choose>
            <xsl:when test="$value != ''">
              <xsl:variable name="Exp" select="substring-after($number, 'E')"/>
              <xsl:variable name="Man" select="substring-before($number, 'E')"/>
              <xsl:variable name="Fac" select="substring('100000000000000000000', 1, substring($Exp,2) + 1)"/>
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

        <xsl:value-of select="$pressure div 100000"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$value"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="time2sec">
    <xsl:param name="time"/>

    <xsl:value-of select="substring-before($time, ':') * 60 + substring-before(substring-after($time, ':'), ' ')"/>
  </xsl:template>

  <xsl:template name="sec2time">
    <xsl:param name="timeSec"/>

    <xsl:value-of select="concat(floor($timeSec div 60), ':', format-number($timeSec mod 60, '00'))"/>
  </xsl:template>

  <!-- Calculate sum of all parameters, and strip any unit following the
       value -->
  <xsl:template name="sum">
    <xsl:param name="values"/>
    <xsl:param name="sum" select="'0'"/>

    <xsl:variable name="value" select="substring-before($values[1], ' ')"/>
    <xsl:choose>

      <!-- No input -->
      <xsl:when test="count($values) = 0"/>

      <!-- Handling last value -->
      <xsl:when test="count($values) = 1">
        <xsl:value-of select="format-number($value + $sum, '#.###')"/>
      </xsl:when>

      <!-- More than one value to sum -->
      <xsl:otherwise>
        <xsl:call-template name="sum">
          <xsl:with-param name="values" select="$values[position() &gt; 1]"/>
          <xsl:with-param name="sum" select="$sum + $value"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Trying to see if we have temperature readings -->
  <xsl:template name="temperatureSamples">
    <xsl:param name="units"/>
    <xsl:choose>
      <xsl:when test="$units = 'Imperial'">
        <xsl:value-of select="count(descendant::temperature[. != 32]|descendant::u:temperature[. != 32]|descendant::u1:temperature[. != 32])"/>
      </xsl:when>
      <xsl:when test="$units = 'Kelvin'">
        <xsl:value-of select="count(descendant::temperature[. != 273.15]|descendant::u:temperature[. != 273.15]|descendant::u1:temperature[. != 273.15])"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="count(descendant::temperature[. != 0]|descendant::u:temperature[. != 0]|descendant::u1:temperature[. != 0])"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="depth2mm">
    <xsl:param name="depth"/>

    <xsl:value-of select="format-number(substring-before($depth, ' '), '#.##') * 1000"/>
  </xsl:template>

  <xsl:template name="mm2depth">
    <xsl:param name="depth"/>

    <xsl:value-of select="concat(floor($depth div 1000), '.', format-number($depth mod 1000, '00'))"/>
  </xsl:template>

  <!-- Convert date format "Sun Jan 19 11:02:56 2014 UTC" => 2014-1-19
       11:02 -->
  <xsl:template name="convertDate">
    <xsl:param name="dateTime"/>

    <xsl:variable name="mnth">
      <xsl:value-of select="substring-before(substring-after($dateTime, ' '), ' ')"/>
    </xsl:variable>

    <xsl:variable name="month">
      <xsl:choose>
        <xsl:when test="$mnth = 'Jan'">1</xsl:when>
        <xsl:when test="$mnth = 'Feb'">2</xsl:when>
        <xsl:when test="$mnth = 'Mar'">3</xsl:when>
        <xsl:when test="$mnth = 'Apr'">4</xsl:when>
        <xsl:when test="$mnth = 'May'">5</xsl:when>
        <xsl:when test="$mnth = 'Jun'">6</xsl:when>
        <xsl:when test="$mnth = 'Jul'">7</xsl:when>
        <xsl:when test="$mnth = 'Aug'">8</xsl:when>
        <xsl:when test="$mnth = 'Sep'">9</xsl:when>
        <xsl:when test="$mnth = 'Oct'">10</xsl:when>
        <xsl:when test="$mnth = 'Nov'">11</xsl:when>
        <xsl:when test="$mnth = 'Dec'">12</xsl:when>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="year">
      <xsl:value-of select="substring-before(substring-after(substring-after($dateTime, ':'), ' '), ' ')"/>
    </xsl:variable>

    <xsl:variable name="day">
      <xsl:value-of select="substring-before(substring-after(substring-after($dateTime, ' '), ' '), ' ')"/>
    </xsl:variable>

    <xsl:variable name="time">
      <xsl:value-of select="substring-before(substring-after(substring-after(substring-after($dateTime, ' '), ' '), ' '), ' ')"/>
    </xsl:variable>

    <xsl:value-of select="concat($year, '-', $month, '-', $day, ' ', $time)"/>
  </xsl:template>

  <xsl:template name="getFieldByIndex">
    <xsl:param name="index"/>
    <xsl:param name="line"/>
    <xsl:param name="remaining"/>
    <xsl:choose>
      <xsl:when test="$index > 0">
        <xsl:choose>
          <xsl:when test="substring($line, 1, 1) = '&quot;'">
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="$index -1"/>
              <xsl:with-param name="line" select="substring-after($line, $fs)"/>
              <xsl:with-param name="remaining" select="$remaining"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="getFieldByIndex">
              <xsl:with-param name="index" select="$index -1"/>
              <xsl:with-param name="line" select="substring-after($line, $fs)"/>
              <xsl:with-param name="remaining" select="$remaining"/>
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="substring($line, 1, 1) = '&quot;'">
            <xsl:choose>
              <xsl:when test="substring-before(substring-after($line, '&quot;'), '&quot;') != ''">
                <xsl:value-of select="substring-before(substring-after($line, '&quot;'), '&quot;')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:choose>
                  <!-- quoted string has new line -->
                  <xsl:when test="string-length(substring-after($line, '&quot;')) = string-length(translate(substring-after($line, '&quot;'), '&#34;', ''))">
                    <xsl:value-of select="concat(substring-after($line, '&quot;'), substring-before($remaining, '&quot;'))"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="''"/>
                  </xsl:otherwise>
                </xsl:choose>
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
