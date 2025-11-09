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

    <xsl:variable name="pressure">
      <xsl:choose>
        <xsl:when test="contains($number, 'E')">
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
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$value"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:value-of select="$pressure div 100000"/>
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

  <!-- CSV Handling -->

  <xsl:variable name="quote" select="'&quot;'"/>

  <!-- Get a field in the first record by index -->

  <xsl:template name="csvGetFieldByIndex">
    <xsl:param name="document"/>
    <xsl:param name="index"/>

    <xsl:choose>
      <xsl:when test="$index > 0">
        <xsl:call-template name="csvGetFieldByIndex">
          <xsl:with-param name="document">
            <xsl:call-template name="csvSkipField">
              <xsl:with-param name="document" select="$document"/>
            </xsl:call-template>
          </xsl:with-param>
          <xsl:with-param name="index" select="$index - 1"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="field">
          <xsl:variable name="fieldLength">
            <xsl:call-template name="csvGetFirstRawFieldLength">
              <xsl:with-param name="document" select="$document"/>
            </xsl:call-template>
          </xsl:variable>

          <xsl:value-of select="substring($document, 1, $fieldLength)"/>
        </xsl:variable>

        <xsl:choose>
          <xsl:when test="starts-with($field, $quote)">
            <xsl:call-template name="csvUnescape">
              <xsl:with-param name="field" select="substring($field, 2, string-length($field) - 2)"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$field"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <!-- Skip to the next record of the document -->

  <xsl:template name="csvSkipRecord">
    <xsl:param name="document"/>

    <xsl:variable name="remaining">
      <xsl:call-template name="csvSkipField">
        <xsl:with-param name="document" select="$document"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:if test="$remaining != ''">
      <xsl:choose>
        <xsl:when test="starts-with($remaining, $lf)">
          <xsl:value-of select="substring($remaining, string-length($lf) + 1)"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="csvSkipRecord">
            <xsl:with-param name="document" select="$remaining"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>


  <!-- Skip to the next field in the current record -->

  <xsl:template name="csvSkipField">
    <xsl:param name="document"/>

    <xsl:variable name="firstFieldLength">
      <xsl:call-template name="csvGetFirstRawFieldLength">
        <xsl:with-param name="document" select="$document"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:choose>
      <xsl:when test="starts-with(substring($document, $firstFieldLength + 1), $fs)">
        <xsl:value-of select="substring($document, $firstFieldLength + 2)"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring($document, $firstFieldLength + 1)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <!-- Get the length of the first field of the current record exactly as it is in the document -->

  <xsl:template name="csvGetFirstRawFieldLength">
    <xsl:param name="document"/>

    <xsl:choose>
      <xsl:when test="starts-with($document, $quote)">
        <xsl:variable name="quotedFieldLength">
          <xsl:call-template name="csvGetQuotedFieldLength">
            <xsl:with-param name="document" select="substring($document, 2)"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:value-of select="$quotedFieldLength + 1"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="firstLine">
          <xsl:choose>
            <xsl:when test="contains($document, $lf)">
              <xsl:value-of select="substring-before($document, $lf)"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$document"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <xsl:choose>
          <xsl:when test="contains($firstLine, $fs)">
            <xsl:value-of select="string-length(substring-before($firstLine, $fs))"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="string-length($firstLine)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <!-- Get the remaining length of the partial quoted field at the beginning of the document -->

  <xsl:template name="csvGetQuotedFieldLength">
    <xsl:param name="document"/>
    <xsl:param name="afterQuote" select="false()"/>

    <xsl:choose>
      <xsl:when test="not($afterQuote)">
        <xsl:variable name="lengthBeforeQuote" select="string-length(substring-before($document, $quote))"/>
        <xsl:variable name="quotedFieldLength">
          <xsl:call-template name="csvGetQuotedFieldLength">
            <xsl:with-param name="document" select="substring($document, $lengthBeforeQuote + 2)"/>
            <xsl:with-param name="afterQuote" select="true()"/>
          </xsl:call-template>
        </xsl:variable>

        <xsl:value-of select="$quotedFieldLength + $lengthBeforeQuote + 1"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="starts-with($document, $fs) or starts-with($document, $lf) or (string-length($document) = 0)">
            <xsl:value-of select="0"/>
          </xsl:when>
          <xsl:when test="starts-with($document, $quote)">
            <xsl:variable name="quotedFieldLength">
              <xsl:call-template name="csvGetQuotedFieldLength">
                <xsl:with-param name="document" select="substring($document, 2)"/>
                <xsl:with-param name="afterQuote" select="false()"/>
              </xsl:call-template>
            </xsl:variable>

            <xsl:value-of select="$quotedFieldLength + 1"/>
          </xsl:when>
          <xsl:otherwise>
            <!-- This should not be happening as unescaped double quotes are
                 not allowed inside of quoted fields in RFC 4180.
                 But we'll continue on here treating this as a single quote. -->
            <xsl:variable name="quotedFieldLength">
              <xsl:call-template name="csvGetQuotedFieldLength">
                <xsl:with-param name="document" select="substring($document, 2)"/>
                <xsl:with-param name="afterQuote" select="false()"/>
              </xsl:call-template>
            </xsl:variable>

            <xsl:value-of select="$quotedFieldLength + 1"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>


  <!-- Convert escaped quotes into single quotes -->

  <xsl:template name="csvUnescape">
    <xsl:param name="field"/>

    <xsl:choose>
      <xsl:when test="contains($field, concat($quote, $quote))">
        <xsl:value-of select="concat(substring-before($field, concat($quote, $quote)), $quote)"/>
        <xsl:call-template name="csvUnescape">
          <xsl:with-param name="field" select="substring-after($field, concat($quote, $quote))"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$field"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
