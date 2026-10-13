<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" indent="yes"/>

  <xsl:variable name="is_apd_inspiration">
    <xsl:value-of select="boolean(/uddf/diver/owner/equipment/divecomputer/@id = 'INSPIRATION')"/>
  </xsl:variable>

  <xsl:template match="/">
    <divelog program="subsurface-import" version="2">
      <settings>
        <divecomputerid>
          <xsl:attribute name="deviceid">
            <xsl:value-of select="/uddf/diver/owner/equipment/divecomputer/@id" />
          </xsl:attribute>
          <xsl:attribute name="model">
            <xsl:value-of select="/uddf/diver/owner/equipment/divecomputer/model" />
          </xsl:attribute>
          <xsl:attribute name="nickname">
            <xsl:value-of select="/uddf/diver/owner/equipment/divecomputer/name" />
          </xsl:attribute>
          <xsl:choose>
            <xsl:when test="/UDDF/history != ''">
              <xsl:apply-templates select="/UDDF/history"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="/uddf/generator"/>
            </xsl:otherwise>
          </xsl:choose>
        </divecomputerid>
      </settings>
      <dives>
        <xsl:apply-templates select="/uddf/profiledata/repetitiongroup/dive|/UDDF/dive"/>
      </dives>
    </divelog>
  </xsl:template>

  <xsl:template match="generator">
    <xsl:if test="manufacturer/name != ''">
      <xsl:attribute name="model">
        <xsl:value-of select="name"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="name != ''">
      <xsl:attribute name="firmware">
        <xsl:value-of select="manufacturer/name"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="version != ''">
      <xsl:attribute name="serial">
        <xsl:value-of select="version"/>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

  <xsl:template match="modified">
    <xsl:if test="application/name != ''">
      <xsl:attribute name="model">
        <xsl:value-of select="application/name"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="application/version != ''">
      <xsl:attribute name="serial">
        <xsl:value-of select="application/version"/>
      </xsl:attribute>
    </xsl:if>
  </xsl:template>

  <xsl:template match="dive">
    <xsl:variable name="tankdata">
      <xsl:value-of select="count(//tankdata)"/>
    </xsl:variable>
    <xsl:variable name="divemode">
      <xsl:if test="samples/waypoint/divemode/@type">
        <xsl:choose>
          <xsl:when test="samples/waypoint/divemode[@type = 'closedcircuit']">
            <xsl:text>closedcircuit</xsl:text>
          </xsl:when>
          <xsl:when test="samples/waypoint/divemode[@type = 'semiclosedcircuit']">
            <xsl:text>semiclosedcircuit</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="samples/waypoint/divemode/@type[1]"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="max_num_o2sensors">
      <xsl:choose>
        <!-- On the APD Inspiration, sensor slots 4 to 6 contain the second controller's readings for the first 3 cells.-->
        <xsl:when test="$is_apd_inspiration">
          <xsl:text>3</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>6</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <dive>
      <!-- Count the amount of temeprature samples during the dive -->
      <xsl:variable name="temperatureSamples">
        <xsl:call-template name="temperatureSamples" select="samples/waypoint/temperature|samples/t">
          <xsl:with-param name="units" select="'Kelvin'"/>
        </xsl:call-template>
      </xsl:variable>

      <xsl:choose>
        <xsl:when test="date != ''">
          <xsl:attribute name="date">
            <xsl:value-of select="concat(date/year,'-',format-number(date/month, '00'), '-', format-number(date/day, '00'))"/>
          </xsl:attribute>
          <xsl:attribute name="time">
            <xsl:value-of select="concat(format-number(time/hour, '00'), ':', format-number(time/minute, '00'))"/>
          </xsl:attribute>
        </xsl:when>
        <xsl:when test="informationbeforedive/datetime != ''">
          <xsl:call-template name="datetime">
            <xsl:with-param name="value">
              <xsl:value-of select="informationbeforedive/datetime"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>
      <xsl:if test="dive_number|informationbeforedive/divenumber != ''">
        <xsl:attribute name="number">
          <xsl:value-of select="dive_number|informationbeforedive/divenumber"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:if test="(dive_duration != '' and dive_duration != 0) or (informationafterdive/diveduration != '' and informationafterdive/diveduration != 0)">
        <xsl:attribute name="duration">
          <xsl:call-template name="timeConvert">
            <xsl:with-param name="timeSec">
              <xsl:value-of select="(dive_duration[.&gt;0])|(informationafterdive/diveduration[.&gt;0])"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:attribute>
      </xsl:if>

      <xsl:if test="condition/visibility|informationafterdive/visibility != '' and condition/visibility|informationafterdive/visibility != 0">
        <xsl:attribute name="visibility">
          <xsl:choose>
            <xsl:when test="condition/visibility|informationafterdive/visibility &lt; 1">
              <xsl:value-of select="1"/>
            </xsl:when>
            <xsl:when test="condition/visibility|informationafterdive/visibility &lt;= 3">
              <xsl:value-of select="2"/>
            </xsl:when>
            <xsl:when test="condition/visibility|informationafterdive/visibility &lt;= 5">
              <xsl:value-of select="3"/>
            </xsl:when>
            <xsl:when test="condition/visibility|informationafterdive/visibility &lt;= 10">
              <xsl:value-of select="4"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="5"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </xsl:if>

      <xsl:if test="informationafterdive/rating/ratingvalue != '' and informationafterdive/rating/ratingvalue != 0">
        <xsl:attribute name="rating">
          <xsl:choose>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt; 2">
              <xsl:value-of select="0"/>
            </xsl:when>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt;= 2">
              <xsl:value-of select="1"/>
            </xsl:when>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt;= 4">
              <xsl:value-of select="2"/>
            </xsl:when>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt;= 6">
              <xsl:value-of select="3"/>
            </xsl:when>
            <xsl:when test="informationafterdive/rating/ratingvalue &lt;= 8">
              <xsl:value-of select="4"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="5"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </xsl:if>

      <xsl:if test="dive_site_ref/@ref|informationbeforedive/dive_site_ref/@ref != ''">
        <location>
          <xsl:variable name="ref">
            <xsl:value-of select="dive_site_ref/@ref|informationbeforedive/dive_site_ref/@ref"/>
          </xsl:variable>
          <xsl:if test="//dive_site[@id=$ref]/geography/gps/longitude != ''">
            <xsl:attribute name="gps">
              <xsl:value-of select="concat(//dive_site[@id=$ref]/geography/gps/latitude, ' ', //dive_site[@id=$ref]/geography/gps/longitude)"/>
            </xsl:attribute>
          <xsl:for-each select="//dive_site[@id=$ref]/geography/location|//dive_site[@id=$ref]/name">
            <xsl:value-of select="."/>
            <xsl:if test=". != '' and following-sibling::*[1]/* != ''"> / </xsl:if>
          </xsl:for-each>
          </xsl:if>
        </location>
      </xsl:if>

      <xsl:for-each select="informationbeforedive/link">
        <xsl:variable name="ref">
          <xsl:value-of select="@ref"/>
        </xsl:variable>
        <xsl:if test="//divesite/site[@id = $ref]/name">
          <location>
            <xsl:choose>
              <xsl:when test="//divesite/site[@id=$ref]/geography/longitude != ''">
                <xsl:attribute name="gps">
                  <xsl:value-of select="concat(//divesite/site[@id=$ref]/geography/latitude, ' ', //divesite/site[@id=$ref]/geography/longitude)"/>
                </xsl:attribute>
                <xsl:value-of select="//divesite/site[@id=$ref]/name"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="//divesite/site[@id=$ref]/name"/>
              </xsl:otherwise>
            </xsl:choose>
          </location>
        </xsl:if>
      </xsl:for-each>

      <xsl:variable name="buddies">
              <xsl:for-each select="informationbeforedive/link">
          <xsl:variable name="ref">
            <xsl:value-of select="@ref"/>
          </xsl:variable>
          <xsl:for-each select="//diver/buddy[@id = $ref]/personal/firstname|//diver/buddy[@id = $ref]/personal/lastname">
            <xsl:value-of select="."/>
            <xsl:if test="following-sibling::* != ''">
              <xsl:value-of select="' '"/>
            </xsl:if>
          </xsl:for-each>
          <xsl:variable name="next">
            <xsl:value-of select="following-sibling::link/@ref"/>
          </xsl:variable>
          <xsl:if test="//diver/buddy[@id = $next]/personal/firstname != ''">
            <xsl:value-of select="', '"/>
          </xsl:if>
        </xsl:for-each>
      </xsl:variable>

      <xsl:if test="$buddies != ''">
        <buddy>
          <xsl:value-of select="$buddies"/>
        </buddy>
      </xsl:if>

      <xsl:if test="buddy_ref/@ref|informationbeforedive/buddy_ref/@ref != ''">
        <buddy>
          <xsl:variable name="ref">
            <xsl:value-of select="buddy_ref/@ref|informationbeforedive/buddy_ref/@ref"/>
          </xsl:variable>
          <xsl:for-each select="//diver[@id=$ref]/personal/first_name|//diver[@id=$ref]/personal/nick_name|//diver[@id=$ref]/personal/family_name|//diver/buddy[@id=$ref]/personal/first_name|//diver/buddy[@id=$ref]/personal/nick_name|//diver/buddy[@id=$ref]/personal/family_name">
            <xsl:value-of select="."/>
            <xsl:if test=". != '' and (following-sibling::*[1] != '' or following-sibling::*[2] != '')"> / </xsl:if>
          </xsl:for-each>
        </buddy>
      </xsl:if>

      <xsl:if test="note/text|informationafterdive/notes/para != ''">
        <notes>
          <xsl:value-of select="note/text|informationafterdive/notes/para"/>
        </notes>
      </xsl:if>

      <xsl:if test="equipment_used/weight_used|informationbeforedive/equipmentused/leadquantity|informationafterdive/equipmentused/leadquantity &gt; 0">
        <weightsystem description="unknown">
          <xsl:attribute name="weight">
            <xsl:value-of select="concat(format-number((equipment_used/weight_used[.&gt;0])|(informationbeforedive/equipmentused/leadquantity[.&gt;0])|(informationafterdive/equipmentused/leadquantity[.&gt;0]), '0.0'), ' kg')"/>
          </xsl:attribute>
        </weightsystem>
      </xsl:if>

      <temperature>
        <xsl:for-each select="lowesttemperature|informationafterdive/lowesttemperature|condition/water_temp">
          <xsl:if test="$temperatureSamples &gt; 0 or . != 273.15">
            <xsl:attribute name="water">
              <xsl:value-of select="concat(format-number(.- 273.15, '0.0'), ' C')"/>
            </xsl:attribute>
          </xsl:if>
        </xsl:for-each>
        <xsl:if test="condition/air_temp|informationbeforedive/airtemperature != ''">
          <xsl:attribute name="air">
            <xsl:value-of select="concat(format-number(condition/air_temp|informationbeforedive/airtemperature - 273.15, '0.0'), ' C')"/>
          </xsl:attribute>
        </xsl:if>
      </temperature>

      <xsl:if test="equipment_used/tank_used != ''">
        <xsl:for-each select="equipment_used/tank_used">
          <cylinder>
            <xsl:variable name="idx">
              <xsl:value-of select="./tank_ref/@ref"/>
            </xsl:variable>
            <xsl:variable name="gas">
              <xsl:value-of select="./gas_ref/@ref"/>
            </xsl:variable>
            <xsl:attribute name="size">
              <xsl:value-of select="//equipment[@id=$idx]/tank/volume"/>
            </xsl:attribute>
            <xsl:attribute name="description">
              <xsl:value-of select="//equipment[@id=$idx]/general/name"/>
            </xsl:attribute>
            <xsl:attribute name="o2">
              <xsl:value-of select="//gas_mix[@id=$gas]/o2"/>
            </xsl:attribute>
            <xsl:attribute name="he">
              <xsl:value-of select="//gas_mix[@id=$gas]/he"/>
            </xsl:attribute>
            <xsl:if test="./pressure_start != ''">
              <xsl:attribute name="start">
                <xsl:value-of select="concat(substring-before(./pressure_start, '.') div 100000, ' bar')"/>
              </xsl:attribute>
            </xsl:if>
            <xsl:if test="./pressure_end != ''">
              <xsl:attribute name="end">
                <xsl:value-of select="concat(substring-before(./pressure_end, '.') div 100000, ' bar')"/>
              </xsl:attribute>
            </xsl:if>
          </cylinder>
        </xsl:for-each>
      </xsl:if>

      <xsl:if test="/uddf/gasdefinitions != '' and $tankdata = 0">
        <xsl:for-each select="/uddf/gasdefinitions/mix">
          <cylinder>
            <xsl:attribute name="description">
              <xsl:choose>
                <xsl:when test="name">
                  <xsl:value-of select="name"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:text>unknown</xsl:text>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
            <xsl:attribute name="o2">
              <xsl:value-of select="concat(o2 * 100, '%')"/>
            </xsl:attribute>
            <xsl:if test="he &gt; 0">
              <xsl:attribute name="he">
                <xsl:value-of select="concat(he * 100, '%')"/>
              </xsl:attribute>
            </xsl:if>

            <xsl:if test="$divemode = 'closedcircuit'">
              <xsl:attribute name="use">
                <xsl:text>diluent</xsl:text>
              </xsl:attribute>
            </xsl:if>
          </cylinder>
        </xsl:for-each>
      </xsl:if>

      <xsl:for-each select="tankdata">
        <cylinder>
          <xsl:variable name="gas">
            <xsl:value-of select="link/@ref"/>
          </xsl:variable>

          <xsl:if test="tankvolume != ''">
            <xsl:attribute name="size">
              <xsl:value-of select="number(tankvolume) * 1000"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:attribute name="o2">
            <xsl:value-of select="//gasdefinitions/mix[@id=$gas]/o2"/>
          </xsl:attribute>
          <xsl:attribute name="he">
            <xsl:value-of select="//gasdefinitions/mix[@id=$gas]/he"/>
          </xsl:attribute>

          <xsl:if test="tankpressurebegin != ''">
            <xsl:attribute name="start">
              <xsl:value-of select="concat(format-number(tankpressurebegin div 100000, '#.#'), ' bar')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="tankpressureend != ''">
            <xsl:attribute name="end">
              <xsl:value-of select="concat(format-number(tankpressureend div 100000, '#.#'), ' bar')"/>
            </xsl:attribute>
          </xsl:if>

          <xsl:if test="$divemode = 'closedcircuit'">
            <xsl:attribute name="use">
              <xsl:text>diluent</xsl:text>
            </xsl:attribute>
          </xsl:if>
        </cylinder>
      </xsl:for-each>

      <!-- For CCR dives, find switches to open circuit and gas switches during open circuit segments, and add bailout cylinders for them -->
      <xsl:if test="$divemode = 'closedcircuit'">
        <xsl:for-each select="samples/waypoint[switchmix]">
          <xsl:if test="(preceding-sibling::waypoint[divemode]|self::waypoint[divemode])[last()]/divemode/@type != 'closedcircuit' or (self::waypoint[divemode]|following-sibling::waypoint[divemode])[1]/divemode/@type != 'closedcircuit'">
            <xsl:variable name="reference" select="switchmix/@ref"/>
            <xsl:for-each select="//gasdefinitions/mix[@id=$reference]">
              <cylinder use="OC-gas">
                <xsl:attribute name="description">
                  <xsl:choose>
                    <xsl:when test="name">
                      <xsl:value-of select="concat(name, ' (bailout)')"/>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:text>unknown</xsl:text>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:attribute>
                <xsl:attribute name="o2">
                  <xsl:value-of select="concat(o2 * 100, '%')"/>
                </xsl:attribute>
                <xsl:if test="he &gt; 0">
                  <xsl:attribute name="he">
                    <xsl:value-of select="concat(he * 100, '%')"/>
                  </xsl:attribute>
                </xsl:if>
              </cylinder>
            </xsl:for-each>
          </xsl:if>
        </xsl:for-each>
      </xsl:if>

      <divecomputer>
        <xsl:attribute name="deviceid">
          <xsl:value-of select="/uddf/diver/owner/equipment/divecomputer/@id" />
        </xsl:attribute>
        <xsl:attribute name="model">
          <xsl:choose>
            <xsl:when test="/uddf/diver/owner/equipment/divecomputer/model != ''">
                <xsl:value-of select="/uddf/diver/owner/equipment/divecomputer/model" />
              </xsl:when>
              <xsl:otherwise>
              <xsl:value-of select="/uddf/generator/name|/UDDF/history/modified/application/name"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>

        <!-- Divemode -->
        <xsl:if test="$divemode">
          <xsl:attribute name='dctype'>
            <xsl:call-template name="divemodeConvert">
              <xsl:with-param name="divemode" select="$divemode"/>
            </xsl:call-template>
          </xsl:attribute>
        </xsl:if>

        <xsl:if test="/uddf/generator/name">
          <extradata key="Generator">
            <xsl:attribute name="value">
              <xsl:value-of select="/uddf/generator/name"/>
            </xsl:attribute>
          </extradata>
        </xsl:if>
        <xsl:if test="/uddf/generator/version">
          <extradata key="Generator Version">
            <xsl:attribute name="value">
              <xsl:value-of select="/uddf/generator/version"/>
            </xsl:attribute>
          </extradata>
        </xsl:if>
        <xsl:if test="/uddf/diver/owner/equipment/divecomputer/serialnumber">
          <extradata key="Serial">
            <xsl:attribute name="value">
              <xsl:value-of select="/uddf/diver/owner/equipment/divecomputer/serialnumber"/>
            </xsl:attribute>
          </extradata>
        </xsl:if>
        <xsl:if test="/uddf/diver/owner/equipment/divecomputer/softwareupdate/softwareversion">
          <extradata key="FW Version">
            <xsl:attribute name="value">
              <xsl:value-of select="/uddf/diver/owner/equipment/divecomputer/softwareupdate/softwareversion"/>
            </xsl:attribute>
          </extradata>
        </xsl:if>

        <xsl:variable name="batteryvoltage_values" select="samples/waypoint[batteryvoltage]"/>

        <xsl:for-each select="/uddf/diver/owner/equipment/rebreather/battery">
          <xsl:variable name="reference" select="@id"/>

          <!-- On the APD Inspiration, battery slots 3 and 4 contain the second controller's readings for the batteries.-->
          <xsl:if test="not($is_apd_inspiration and position() &gt; 2)">
            <extradata>
              <xsl:attribute name="key">
                <xsl:value-of select="concat('Battery ', position(), ' at Begin [V]')"/>
              </xsl:attribute>
              <xsl:attribute name="value">
                <xsl:value-of select="$batteryvoltage_values[batteryvoltage[@ref = $reference]][1]/batteryvoltage[@ref = $reference]"/>
              </xsl:attribute>
            </extradata>
            <extradata>
              <xsl:attribute name="key">
                <xsl:value-of select="concat('Battery ', position(), ' at End [V]')"/>
              </xsl:attribute>
              <xsl:attribute name="value">
                <xsl:value-of select="$batteryvoltage_values[batteryvoltage[@ref = $reference]][last()]/batteryvoltage[@ref = $reference]"/>
              </xsl:attribute>
            </extradata>
          </xsl:if>
        </xsl:for-each>

        <xsl:if test="/uddf/diver/owner/equipment/rebreather/scrubbermonitor">
          <xsl:choose>
            <xsl:when test="/uddf/diver/owner/equipment/rebreather/scrubbermonitor/@id = 'tempstik' and samples/waypoint/scrubber[@ref = 'tempstik']">
              <!-- APD Inspiration Tempstik -->
              <extradata key="Tempstik at Begin">
                <xsl:attribute name="value">
                  <xsl:call-template name="getTempstikGraph">
                    <!-- The tempstik readings at the beginning of the dive seem to be bogus -->
                    <xsl:with-param name="tempstik_value" select="samples/waypoint[scrubber[@ref = 'tempstik'] and divetime > 0][1]/scrubber[@ref = 'tempstik']"/>
                  </xsl:call-template>
                </xsl:attribute>
              </extradata>
              <extradata key="Tempstik at End">
                <xsl:attribute name="value">
                  <xsl:call-template name="getTempstikGraph">
                    <xsl:with-param name="tempstik_value" select="samples/waypoint[scrubber[@ref = 'tempstik']][last()]/scrubber[@ref = 'tempstik']"/>
                  </xsl:call-template>
                </xsl:attribute>
              </extradata>
            </xsl:when>
            <xsl:otherwise>
              <xsl:variable name="scrubber_values" select="samples/waypoint[scrubber]"/>
              <xsl:for-each select="/uddf/diver/owner/equipment/rebreather/scrubbermonitor[@id]">
                <xsl:variable name="reference" select="@id"/>

                <extradata key="Scrubber Monitor at Begin">
                  <xsl:attribute name="value">
                    <xsl:value-of select="$scrubber_values[scrubber[@ref = $reference]][1]/scrubber[@ref = $reference]"/>
                    <xsl:if test="$scrubber_values[scrubber[@ref = $reference]][1]/scrubber[@ref = $reference]/@units != ''">
                      <xsl:value-of select="concat(' ', $scrubber_values[scrubber[@ref = $reference]][1]/scrubber[@ref = $reference]/@units)"/>
                    </xsl:if>
                  </xsl:attribute>
                </extradata>
                <extradata key="Scrubber Monitor at End">
                  <xsl:attribute name="value">
                    <xsl:value-of select="$scrubber_values[scrubber[@ref = $reference]][last()]/scrubber[@ref = $reference]"/>
                    <xsl:if test="$scrubber_values[scrubber[@ref = $reference]][last()]/scrubber[@ref = $reference]/@units != ''">
                      <xsl:value-of select="concat(' ', $scrubber_values[scrubber[@ref = $reference]][last()]/scrubber[@ref = $reference]/@units)"/>
                    </xsl:if>
                  </xsl:attribute>
                </extradata>
              </xsl:for-each>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:if>

        <xsl:if test="/uddf/diver/owner/equipment/*[self::divecomputer or self::rebreather]/timerdevice">
          <xsl:choose>
            <xsl:when test="/uddf/diver/owner/equipment/rebreather/timerdevice/@id = 'elapsed_on_time' and informationafterdive/timer[@ref = 'elapsed_on_time']">
              <!-- APD Inspiration 'Elapsed on time' timer -->
              <extradata key="Elapsed on time at End [hh:mm:ss]">
                <xsl:attribute name="value">
                  <xsl:call-template name="timeConvertHours">
                    <xsl:with-param name="timeSec" select="informationafterdive/timer[@ref = 'elapsed_on_time']"/>
                  </xsl:call-template>
                </xsl:attribute>
              </extradata>
            </xsl:when>
            <xsl:otherwise>
              <xsl:variable name="timer_values" select="*[self::informationbeforedive or self::informationafterdive][timer]"/>

              <xsl:for-each select="/uddf/diver/owner/equipment/*[self::divecomputer or self::rebreather]/timerdevice[@id]">
                <xsl:variable name="reference" select="@id"/>

                <xsl:if test="$timer_values[self::informationbeforedive]/timer[@ref = $reference]">
                  <extradata>
                    <xsl:attribute name="key">
                      <xsl:value-of select="concat($reference, ' at Begin [hh:mm:ss]')"/>
                    </xsl:attribute>
                    <xsl:attribute name="value">
                      <xsl:call-template name="timeConvertHours">
                        <xsl:with-param name="timeSec" select="$timer_values[self::informationbeforedive]/timer[@ref = $reference]"/>
                      </xsl:call-template>
                    </xsl:attribute>
                  </extradata>
                </xsl:if>
                <xsl:if test="$timer_values[self::informationafterdive]/timer[@ref = $reference]">
                  <extradata>
                    <xsl:attribute name="key">
                      <xsl:value-of select="concat($reference, ' at End [hh:mm:ss]')"/>
                    </xsl:attribute>
                    <xsl:attribute name="value">
                      <xsl:call-template name="timeConvertHours">
                        <xsl:with-param name="timeSec" select="$timer_values[self::informationafterdive]/timer[@ref = $reference]"/>
                      </xsl:call-template>
                    </xsl:attribute>
                  </extradata>
                </xsl:if>
              </xsl:for-each>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:if>

        <xsl:if test="samples/waypoint/setgflow and samples/waypoint/setgfhigh">
          <!-- APD Inspiration gradient factors - this is not part of the UDDF standard -->
          <extradata key="Deco model">
            <xsl:attribute name="value">
              <xsl:value-of select="concat('GF ', round(samples/waypoint[setgflow and divetime > 0][1]/setgflow * 100), '/', round(samples/waypoint[setgfhigh and divetime > 0][1]/setgfhigh * 100))"/>
                <!-- The gradient factor readings at the beginning of the dive seem to be bogus -->
            </xsl:attribute>
          </extradata>
        </xsl:if>

        <xsl:if test="$is_apd_inspiration and informationbeforedive/divenumber">
          <!-- APD Inspiration unit dive number - this is a non-user-modifiable counter -->
          <extradata key="Unit dive number">
            <xsl:attribute name="value">
              <xsl:value-of select="informationbeforedive/divenumber"/>
            </xsl:attribute>
          </extradata>
        </xsl:if>

        <depth>
          <xsl:for-each select="greatestdepth|informationafterdive/greatestdepth|max_depth">
            <xsl:attribute name="max">
              <xsl:value-of select="concat(format-number(., '0.00'), ' m')"/>
            </xsl:attribute>
          </xsl:for-each>
          <xsl:for-each select="averagedepth|informationafterdive/averagedepth">
            <xsl:attribute name="mean">
              <xsl:value-of select="concat(format-number(., '0.00'), ' m')"/>
            </xsl:attribute>
          </xsl:for-each>
        </depth>

        <!-- Aquadivelog gas switches require more lookups than other UDDF
             formats I have seen -->
        <xsl:for-each select="samples/switch">
          <xsl:variable name="tank_idx">
            <xsl:value-of select="./@tank"/>
          </xsl:variable>
          <xsl:variable name="idx">
            <xsl:value-of select="../../equipment_used/tank_used[@id=$tank_idx]/gas_ref/@ref"/>
          </xsl:variable>

          <xsl:variable name="type">
            <xsl:choose>
              <xsl:when test="translate(//gas_def/gas_mix[@id=$idx]/o2, ',', '.') &gt; 0">
                <xsl:value-of select="25"/> <!-- SAMPLE_EVENT_GASCHANGE2 -->
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="11"/> <!-- SAMPLE_EVENT_GASCHANGE -->
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>

          <event name="gaschange" type="{$type}">
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="following-sibling::t"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>

            <xsl:attribute name="o2">
              <xsl:call-template name="gasConvert">
                <xsl:with-param name="mix">
                  <xsl:value-of select="translate(//gas_def/gas_mix[@id=$idx]/o2, ',', '.')"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>
          </event>
        </xsl:for-each>

        <!-- Other gas switches than Aquadivelog -->
        <!-- Waypoints that also carry divemode are handled below, by the
             "gaschange events ... when switching divemode" loop, which
             already derives the same gas reference and also sets the
             divemode/flags attributes; excluding them here avoids emitting
             the same gas-change event twice for those waypoints. -->
        <xsl:for-each select="samples/waypoint[switchmix and not(divemode)]">
          <!-- Index to look up gas per cent -->
          <xsl:variable name="reference" select="switchmix/@ref"/>
          <xsl:variable name="helium_fraction" select="translate(//gasdefinitions/mix[@id=$reference]/he, ',', '.')"/>

          <xsl:variable name="type">
            <xsl:choose>
              <xsl:when test="$helium_fraction &gt; 0">
                <xsl:value-of select="25"/> <!-- SAMPLE_EVENT_GASCHANGE2 -->
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="11"/> <!-- SAMPLE_EVENT_GASCHANGE -->
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>

          <event name="gaschange" type="{$type}">
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="divetime"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>

            <xsl:attribute name="o2">
              <xsl:call-template name="gasConvert">
                <xsl:with-param name="mix">
                  <xsl:value-of select="translate(//gasdefinitions/mix[@id=$reference]/o2, ',', '.')"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>

            <xsl:if test="$helium_fraction &gt; 0">
              <xsl:attribute name="he">
                <xsl:call-template name="gasConvert">
                  <xsl:with-param name="mix">
                    <xsl:value-of select="$helium_fraction"/>
                  </xsl:with-param>
                </xsl:call-template>
              </xsl:attribute>
            </xsl:if>

            <xsl:if test="$divemode = 'closedcircuit'">
              <xsl:attribute name="divemode">
                <xsl:choose>
                  <xsl:when test="(preceding-sibling::waypoint[divemode]|self::waypoint[divemode])[last()]/divemode/@type != 'closedcircuit'">
                    <xsl:text>OC</xsl:text>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:text>CCR</xsl:text>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:attribute>

              <xsl:attribute name="flags">
                <xsl:text>1</xsl:text>
              </xsl:attribute>
            </xsl:if>
          </event>
        </xsl:for-each>

        <xsl:for-each select="samples/waypoint/alarm">
          <event>
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="preceding-sibling::divetime|following-sibling::divetime"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>

            <xsl:attribute name="name">
              <xsl:value-of select="."/>
            </xsl:attribute>
          </event>
        </xsl:for-each>

        <xsl:for-each select="samples/waypoint/heading">
          <event name="heading">
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="preceding-sibling::divetime"/>
                </xsl:with-param>
            </xsl:call-template>
          </xsl:attribute>

          <xsl:attribute name="value">
              <xsl:value-of select="."/>
            </xsl:attribute>
          </event>
        </xsl:for-each>

        <!-- Add gaschange events to a cylinder of the appropriate use type when switching divemode -->
        <xsl:for-each select="samples/waypoint[divemode]">
          <xsl:variable name="reference" select="(preceding-sibling::waypoint[switchmix]|self::waypoint[switchmix])[last()]/switchmix/@ref"/>
          <xsl:variable name="type">
            <xsl:choose>
              <xsl:when test="translate(//gasdefinitions/mix[@id=$reference]/he, ',', '.') &gt; 0">
                <xsl:value-of select="25"/> <!-- SAMPLE_EVENT_GASCHANGE2 -->
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="11"/> <!-- SAMPLE_EVENT_GASCHANGE -->
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>

          <event name="gaschange" type="{$type}">
            <xsl:attribute name="time">
              <xsl:call-template name="timeConvert">
                <xsl:with-param name="timeSec">
                  <xsl:value-of select="divetime"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>

            <xsl:attribute name="o2">
              <xsl:call-template name="gasConvert">
                <xsl:with-param name="mix">
                  <xsl:value-of select="translate(//gasdefinitions/mix[@id=$reference]/o2, ',', '.')"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>

            <xsl:attribute name="he">
              <xsl:call-template name="gasConvert">
                <xsl:with-param name="mix">
                  <xsl:value-of select="translate(//gasdefinitions/mix[@id=$reference]/he, ',', '.')"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>

            <xsl:attribute name="divemode">
              <xsl:call-template name="divemodeConvert">
                <xsl:with-param name="divemode">
                  <xsl:value-of select="divemode/@type"/>
                </xsl:with-param>
              </xsl:call-template>
            </xsl:attribute>

            <xsl:attribute name="flags">
              <xsl:text>1</xsl:text>
            </xsl:attribute>
          </event>
        </xsl:for-each>

        <xsl:for-each select="samples/waypoint|samples/d">
          <xsl:if test="./depth != ''">
            <sample>
              <xsl:attribute name="time">
                <xsl:call-template name="timeConvert">
                  <xsl:with-param name="timeSec">
                    <xsl:value-of select="divetime|preceding-sibling::t[1]"/>
                  </xsl:with-param>
                </xsl:call-template>
              </xsl:attribute>

              <xsl:choose>
                <xsl:when test="depth != ''">
                  <xsl:attribute name="depth">
                    <xsl:value-of select="concat(format-number(translate(depth, ',', '.'), '0.00'), ' m')"/>
                  </xsl:attribute>
                </xsl:when>
                <xsl:when test=". != 0">
                  <xsl:attribute name="depth">
                    <xsl:value-of select="concat(format-number(translate(., ',', '.'), '0.00'), ' m')"/>
                  </xsl:attribute>
                </xsl:when>
              </xsl:choose>

              <xsl:if test="temperature != '' and $temperatureSamples &gt; 0">
                <xsl:attribute name="temp">
                  <xsl:value-of select="concat(format-number(temperature - 273.15, '0.0'), ' C')"/>
                </xsl:attribute>
              </xsl:if>

              <xsl:if test="tankpressure != ''">
                <xsl:attribute name="pressure">
                  <xsl:value-of select="concat(format-number(tankpressure div 100000, '0.0'), ' bar')"/>
                </xsl:attribute>
              </xsl:if>

              <xsl:if test="otu &gt; 0">
                <xsl:attribute name="otu">
                  <xsl:value-of select="otu"/>
                </xsl:attribute>
              </xsl:if>

              <xsl:if test="cns &gt; 0">
                <xsl:attribute name="cns">
                  <xsl:value-of select="cns"/>
                </xsl:attribute>
              </xsl:if>

              <xsl:if test="setpo2 != ''">
                <xsl:attribute name="po2">
                  <xsl:call-template name="convertPascal">
                    <xsl:with-param name="value">
                      <xsl:value-of select="setpo2"/>
                    </xsl:with-param>
                  </xsl:call-template>
                </xsl:attribute>
              </xsl:if>

              <xsl:variable name="ppo2_values" select="ppo2"/>
              <xsl:for-each select="/uddf/diver/owner/equipment/rebreather/o2sensor">
                <xsl:variable name="reference" select="@id"/>
                <xsl:if test="position() &lt;= $max_num_o2sensors and $ppo2_values[@ref = $reference]">
                  <xsl:variable name="sensor_name" select="concat('sensor', position())"/>
                  <xsl:attribute name="{$sensor_name}">
                    <xsl:call-template name="convertPascal">
                      <xsl:with-param name="value">
                        <xsl:value-of select="$ppo2_values[@ref = $reference]"/>
                      </xsl:with-param>
                    </xsl:call-template>
                  </xsl:attribute>
                </xsl:if>
              </xsl:for-each>

              <xsl:if test="nodecotime &gt; 0">
                <xsl:attribute name="ndl">
                  <xsl:call-template name="timeConvert">
                    <xsl:with-param name="timeSec">
                      <xsl:value-of select="nodecotime"/>
                    </xsl:with-param>
                  </xsl:call-template>
                </xsl:attribute>
              </xsl:if>

              <xsl:if test="decostop">
                <xsl:attribute name="stoptime">
                  <xsl:call-template name="timeConvert">
                    <xsl:with-param name="timeSec">
                      <xsl:value-of select="decostop/@duration"/>
                    </xsl:with-param>
                  </xsl:call-template>
                </xsl:attribute>
                <xsl:attribute name="stopdepth">
                  <xsl:value-of select="decostop/@decodepth"/>
                </xsl:attribute>
                <xsl:attribute name="in_deco">
                  <xsl:choose>
                    <xsl:when test="decostop/@kind != 'mandatory'">
                      <xsl:value-of select="0"/>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:value-of select="1"/>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:attribute>
              </xsl:if>
            </sample>
          </xsl:if>
        </xsl:for-each>
      </divecomputer>
    </dive>
  </xsl:template>

  <!-- convert time in seconds to minutes:seconds -->
  <xsl:template name="timeConvert">
    <xsl:param name="timeSec"/>
    <xsl:if test="$timeSec != ''">
      <xsl:value-of select="concat(floor(number($timeSec) div 60), ':', format-number(floor(number($timeSec) mod 60), '00'), ' min')"/>
    </xsl:if>
  </xsl:template>

  <!-- convert time in seconds to hours:minutes:seconds -->
  <xsl:template name="timeConvertHours">
    <xsl:param name="timeSec"/>
    <xsl:if test="$timeSec != ''">
      <xsl:value-of select="concat(floor(number($timeSec) div 3600), ':', format-number(floor(number($timeSec) mod 3600 div 60), '00'), ':', format-number(floor(number($timeSec) mod 60), '00'))"/>
    </xsl:if>
  </xsl:template>
  <!-- end convert time -->

  <!-- convert gas -->
  <xsl:template name="gasConvert">
    <xsl:param name="mix"/>
    <xsl:if test="$mix != ''">
      <xsl:choose>
        <xsl:when test="$mix &gt;= 0">
          <xsl:choose>
            <xsl:when test="$mix &gt; 1">
              <xsl:value-of select="concat($mix, '%')"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat($mix * 100, '%')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$mix"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>
  <!-- end convert gas -->

  <xsl:template name="divemodeConvert">
    <xsl:param name="divemode"/>
    <xsl:choose>
      <xsl:when test="$divemode = 'closedcircuit'">
        <xsl:text>CCR</xsl:text>
      </xsl:when>
      <xsl:when test="$divemode = 'semiclosedcircuit'">
        <xsl:text>PSCR</xsl:text>
      </xsl:when>
      <xsl:when test="$divemode = 'apnea' or $divemode = 'apnoe'">
        <xsl:text>Freedive</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>OC</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="getTempstikGraph">
    <xsl:param name="tempstik_value"/>
    <xsl:variable name="tempstik_elements">
      <xsl:call-template name="getTempstikElement">
        <xsl:with-param name="tempstik_value_remainder">
          <xsl:value-of select="$tempstik_value"/>
        </xsl:with-param>
        <xsl:with-param name="position">
          <xsl:value-of select="0"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="concat('[', $tempstik_elements, ']')"/>
  </xsl:template>

  <xsl:template name="getTempstikElement">
    <xsl:param name="tempstik_value_remainder"/>
    <xsl:param name="position"/>
    <xsl:if test="$position &lt; 6">
      <xsl:variable name="element_value">
        <xsl:choose>
          <xsl:when test="$tempstik_value_remainder mod 2 = 1">
            <xsl:text>#</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>_</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="tempstik_tail">
        <xsl:call-template name="getTempstikElement">
          <xsl:with-param name="tempstik_value_remainder">
            <xsl:value-of select="floor($tempstik_value_remainder div 2)"/>
          </xsl:with-param>
          <xsl:with-param name="position">
            <xsl:value-of select="$position + 1"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat($element_value, $tempstik_tail)"/>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
