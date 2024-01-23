<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xt="http://www.jclark.com/xt"
                extension-element-prefixes="xt" version="1.0">
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" encoding="utf-8" indent="yes"/>
  <xsl:param name="units" select="units"/>

  <xsl:key name="gases" match="cylinder" use="concat(number(substring-before(@o2, '%')), '/', number(substring-before(@he, '%')))" />
  <xsl:key name="images" match="picture" use="concat(../../dive/@number|../dive/@number, ':', @filename, '@', @offset)" />
  <xsl:key name="divecomputer" match="divecomputer" use="@deviceid" />

  <!-- This needs to be set at this top level so that it is avialable in both the buddies and profiledata sections-->
  <xsl:variable name="buddies">
    <xsl:for-each select="//buddy">
      <xsl:call-template name="tokenize">
        <xsl:with-param name="string" select="." />
        <xsl:with-param name="delim" select="', '" />
      </xsl:call-template>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="eventmap">
    <!--entry key="safety stop (mandatory)"></entry-->
    <!--entry key="deco"></entry-->
    <entry key="ascent">ascent</entry><!--Not sure of definitions in our file. Ascent too fast??-->
    <entry key="violation">deco</entry><!--Assume this is missed deco-->
    <!--entry key="below floor">error</entry-->
    <entry key="divetime">rbt</entry>
    <!--entry key="maxdepth"></entry-->
    <!--entry key="OLF"></entry-->
    <!--entry key="PO2"></entry-->
    <!--entry key="airtime"></entry-->
    <entry key="ceiling">error</entry>
    <!--entry key="heading"></entry-->
    <entry key="surface">surface</entry>
    <!--entry key="bookmark"></entry-->
    <entry key="unknown">error</entry>
  </xsl:variable>


  <xsl:template match="/divelog/settings"/>
  <xsl:template match="/divelog/divesites"/>

  <xsl:template match="/divelog/dives">
    <uddf version="3.2.0"  xmlns="http://www.streit.cc/uddf/3.2/">
      <generator>
        <name>Subsurface Divelog</name>
        <manufacturer id="subsurface">
          <name>Subsurface Team</name>
          <contact>
            <homepage>http://subsurface-divelog.org/</homepage>
          </contact>
        </manufacturer>
        <version>
          <xsl:value-of select="/divelog/@version"/>
        </version>
        <xsl:if test="/divelog/generator/@date != ''">
          <datetime>
            <xsl:value-of select="concat(/divelog/generator/@date, 'T', /divelog/generator/@time)"/>
          </datetime>
        </xsl:if>
      </generator>
      <mediadata>
        <xsl:for-each select="//picture[generate-id() = generate-id(key('images', concat(../../dive/@number|../dive/@number, ':', @filename, '@', @offset))[1])]">
          <image id="{concat(../../dive/@number|../dive/@number, ':', @filename, '@', @offset)}">
            <objectname>
              <xsl:value-of select="@filename"/>
            </objectname>
          </image>
        </xsl:for-each>
      </mediadata>
      <diver>
        <owner id="owner">
          <personal>
            <firstname></firstname>
            <lastname></lastname>
          </personal>
          <equipment>
            <xsl:for-each select="//dive/divecomputer[generate-id() = generate-id(key('divecomputer', @deviceid)[1])]">
              <divecomputer id="{./@deviceid}">
                <name>
                  <xsl:choose>
                    <xsl:when test="./@nickname != ''">
                      <xsl:value-of select="./@nickname"/>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:value-of select="./@model"/>
                    </xsl:otherwise>
                  </xsl:choose>
                </name>
                <model>
                  <xsl:value-of select="./@model"/>
                </model>
              </divecomputer>
            </xsl:for-each>
          </equipment>
        </owner>
        <xsl:for-each select="xt:node-set($buddies)/token[generate-id() = generate-id(key('tokenkey', .)[1])]">
          <xsl:sort select="." />
          <buddy>
            <xsl:attribute name="id">
              <xsl:value-of select="generate-id(key('tokenkey', .)[1])"/>
            </xsl:attribute>
            <personal>
              <xsl:choose>
                <xsl:when test="contains(., ' ')">
                  <firstname>
                    <xsl:value-of select="substring-before(., ' ')"/>
                  </firstname>
                  <lastname>
                    <xsl:value-of select="substring-after(., ' ')"/>
                  </lastname>
                </xsl:when>
                <xsl:otherwise>
                  <firstname>
                    <xsl:value-of select="."/>
                  </firstname>
                </xsl:otherwise>
              </xsl:choose>
            </personal>
          </buddy>
        </xsl:for-each>
        </diver>

      <divesite>
        <!-- There must be at least one divebase. Subsurface doesn't track this as a concept, so just assign them all to a single divebase. -->
        <divebase id="allbase">
          <name>Subsurface Divebase</name>
        </divebase>

        <xsl:apply-templates select="//site" mode="called"/>
      </divesite>

      <!-- Define all the unique gases found in the dive log -->
      <gasdefinitions>
        <!-- Get unique gas mixes from all the recorded dives -->
        <xsl:for-each select="//dive/cylinder[generate-id() = generate-id(key('gases', concat(number(substring-before(@o2, '%')), '/', number(substring-before(@he, '%'))))[1])]">

          <xsl:variable name="o2">
            <xsl:choose>
              <xsl:when test="@o2 != ''">
                <xsl:value-of select="number(substring-before(@o2, '%'))"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="21"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>

          <xsl:variable name="he">
            <xsl:choose>
              <xsl:when test="@he != ''">
                <xsl:value-of select="number(substring-before(@he, '%'))"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="0"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>

          <!-- The gas definitions will have the o2 percentage as mix ID
               to ease up the reference on switchmix events. Thus we can
               just use the same references used internally on
               Subsurface.
          -->
          <mix id="mix({$o2}/{$he})">
            <name>
              <xsl:choose>
                <xsl:when test="$he + $o2 &gt; 100">
                  <xsl:value-of select="concat('Impossible mix: ', $o2, '/', $he)"/>
                </xsl:when>
                <xsl:when test="$he &gt; 0">
                  <xsl:value-of select="concat('TMx ', $o2, '/', $he)"/>
                </xsl:when>
                <xsl:when test="$o2 = 21">
                  <xsl:value-of select="'air'"/>
                </xsl:when>
                <xsl:when test="$o2 = 100">
                  <xsl:value-of select="'oxygen'"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="concat('EANx ', $o2)"/>
                </xsl:otherwise>
              </xsl:choose>
            </name>
            <o2>
              <xsl:value-of select="format-number($o2 div 100, '0.00')"/>
            </o2>
            <he>
              <xsl:value-of select="format-number($he div 100, '0.00')"/>
            </he>
          </mix>
        </xsl:for-each>
      </gasdefinitions>

      <profiledata>

        <xsl:for-each select="trip">
          <repetitiongroup>
            <xsl:attribute name="id">
              <xsl:choose>
                <xsl:when test="$test != ''">
                  <xsl:value-of select="generate-id(.)" />
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="'testid1'" />
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>

            <xsl:apply-templates select="dive"/>
          </repetitiongroup>
        </xsl:for-each>
        <xsl:for-each select="dive">
          <repetitiongroup>
            <xsl:attribute name="id">
              <xsl:choose>
                <xsl:when test="string-length($units) = 0 or $units = 0">
                  <xsl:value-of select="generate-id(.)" />
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="'testid2'" />
                </xsl:otherwise>
              </xsl:choose>
            </xsl:attribute>
            <xsl:apply-templates select="."/>
          </repetitiongroup>
        </xsl:for-each>
      </profiledata>

      <divetrip>
        <xsl:apply-templates select="//trip"/>
      </divetrip>

    </uddf>
  </xsl:template>

  <xsl:key name="tokenkey" match="token" use="." />

  <xsl:template name="tokenize">
    <xsl:param name="string" />
    <xsl:param name="delim" />
    <xsl:choose>
      <xsl:when test="contains($string, $delim)">
      <token><xsl:value-of select="substring-before($string, $delim)" /></token>
        <xsl:call-template name="tokenize">
        <xsl:with-param name="string" select="substring-after($string,$delim)" />
        <xsl:with-param name="delim" select="$delim" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <token><xsl:value-of select="$string" /></token>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="site" mode="called">
    <site xmlns="http://www.streit.cc/uddf/3.2/">
      <xsl:attribute name="id">
        <xsl:value-of select="@uuid"/>
      </xsl:attribute>
      <name>
        <xsl:value-of select="@name"/>
      </name>
      <geography>
        <location>
          <xsl:value-of select="@name"/>
        </location>
        <latitude>
          <xsl:value-of select="substring-before(@gps, ' ')"/>
        </latitude>
        <longitude>
          <xsl:value-of select="substring-after(@gps, ' ')"/>
        </longitude>
      </geography>
      <xsl:if test="notes != ''">
        <sitedata>
          <notes>
            <xsl:value-of select="notes"/>
          </notes>
        </sitedata>
      </xsl:if>
    </site>
  </xsl:template>

  <xsl:template match="dive">
    <dive xmlns="http://www.streit.cc/uddf/3.2/">
      <xsl:attribute name="id">
        <xsl:choose>
          <xsl:when test="string-length($units) = 0 or $units = 0">
            <xsl:value-of select="generate-id(.)" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="'testid3'" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>


      <informationbeforedive>
        <xsl:variable name="buddylist">
          <xsl:call-template name="tokenize">
            <xsl:with-param name="string" select="buddy" />
            <xsl:with-param name="delim" select="', '" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:for-each select="xt:node-set($buddylist)/token">
          <xsl:variable name="buddyname" select="."/>
          <xsl:for-each select="xt:node-set($buddies)/token[generate-id() = generate-id(key('tokenkey', .)[1]) and $buddyname = .]">
          <link>
            <xsl:attribute name="ref">
              <xsl:value-of select="generate-id()"/>
            </xsl:attribute>
          </link>
          </xsl:for-each>
        </xsl:for-each>
        <link>
          <xsl:attribute name="ref">
            <xsl:value-of select="@divesiteid"/>
          </xsl:attribute>
        </link>
        <divenumber>
          <xsl:value-of select="./@number"/>
        </divenumber>
        <datetime>
          <xsl:value-of select="concat(./@date, 'T', ./@time)"/>
        </datetime>
        <xsl:for-each select="divecomputer/temperature/@air|divecomputer/divetemperature/@air|divetemperature/@air">
          <airtemperature>
            <xsl:value-of select="format-number(substring-before(., ' ') + 273.15, '0.00')"/>
          </airtemperature>
        </xsl:for-each>
        <xsl:variable name="trimmedweightlist">
          <xsl:for-each select="weightsystem">
            <weight>
              <xsl:value-of select="substring-before(@weight, ' ')"/>
            </weight>
          </xsl:for-each>
        </xsl:variable>
        <xsl:if test="sum(xt:node-set($trimmedweightlist)/node()) >= 0">
          <equipmentused>
            <leadquantity>
              <xsl:value-of select="sum(xt:node-set($trimmedweightlist)/node())"/>
            </leadquantity>
          </equipmentused>
        </xsl:if>
        <xsl:if test="parent::trip">
          <tripmembership ref="trip{generate-id(..)}"/>
        </xsl:if>
      </informationbeforedive>

      <xsl:for-each select="cylinder">

        <xsl:variable name="o2">
          <xsl:choose>
            <xsl:when test="@o2 != ''">
              <xsl:value-of select="number(substring-before(@o2, '%'))"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="21"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <xsl:variable name="he">
          <xsl:choose>
            <xsl:when test="@he != ''">
              <xsl:value-of select="number(substring-before(@he, '%'))"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="0"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <tankdata>
          <link ref="mix({$o2}/{$he})"/>

          <xsl:if test="@size">

            <tankvolume>
              <xsl:value-of select="substring-before(@size, ' ')"/>
            </tankvolume>
          </xsl:if>
          <xsl:choose>
            <xsl:when test="@start">
              <tankpressurebegin>
                <xsl:value-of select="substring-before(@start, ' ') * 100000"/>
              </tankpressurebegin>
            </xsl:when>
            <xsl:when test="../divecomputer[1]/sample[@pressure]/@pressure[1]">
              <tankpressurebegin>
                <xsl:value-of select="substring-before(../divecomputer[1]/sample[@pressure]/@pressure[1], ' ') * 100000"/>
              </tankpressurebegin>
            </xsl:when>
          </xsl:choose>

          <xsl:choose>
            <xsl:when test="@end">
              <tankpressureend>
                <xsl:value-of select="substring-before(@end, ' ') * 100000"/>
              </tankpressureend>
            </xsl:when>
            <xsl:when test="../divecomputer[1]/sample[@pressure][last()]/@pressure[1]">
              <tankpressureend>
                <xsl:value-of select="substring-before(../divecomputer[1]/sample[@pressure][last()]/@pressure[1], ' ') * 100000"/>
              </tankpressureend>
            </xsl:when>
          </xsl:choose>

        </tankdata>
      </xsl:for-each>

      <samples>

        <xsl:for-each select="divecomputer[1]/event | divecomputer[1]/sample">
          <xsl:sort select="substring-before(@time, ':') * 60 + substring-before(substring-after(@time, ':'), ' ')" data-type="number" order="ascending"/>

        <xsl:variable name="events">
          <xsl:value-of select="count(preceding-sibling::event)"/>
        </xsl:variable>

          <xsl:choose>
            <xsl:when test="name() = 'event'">

              <xsl:variable name="position">
                <xsl:value-of select="position()"/>
              </xsl:variable>

              <!-- Times of surrounding waypoints -->
              <xsl:variable name="timefirst">
                <xsl:choose>
                  <xsl:when test="../sample[position() = $position - 1 - $events]/@time != ''">
                    <xsl:call-template name="time2sec">
                      <xsl:with-param name="time">
                        <xsl:value-of select="../sample[position() = $position - 1 - $events]/@time"/>
                      </xsl:with-param>
                    </xsl:call-template>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="0"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <xsl:variable name="timesecond">
                <xsl:choose>
                  <xsl:when test="../sample[position() = $position - $events]/@time != ''">
                    <xsl:call-template name="time2sec">
                      <xsl:with-param name="time">
                        <xsl:value-of select="../sample[position() = $position - $events]/@time"/>
                      </xsl:with-param>
                    </xsl:call-template>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="0"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:variable>

              <!-- Depths of surrounding waypoints -->
              <xsl:variable name="depthfirst">
                <xsl:choose>
                  <xsl:when test="../sample[position() = $position - 1 - $events]/@depth != ''">
                    <xsl:call-template name="depth2mm">
                      <xsl:with-param name="depth">
                        <xsl:value-of select="../sample[position() = $position - 1 - $events]/@depth"/>
                      </xsl:with-param>
                    </xsl:call-template>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="0"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <xsl:variable name="depthsecond">
                <xsl:choose>
                  <xsl:when test="../sample[position() = $position - $events]/@depth != ''">
                    <xsl:call-template name="depth2mm">
                      <xsl:with-param name="depth">
                        <xsl:value-of select="../sample[position() = $position - $events]/@depth"/>
                      </xsl:with-param>
                    </xsl:call-template>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:value-of select="0"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:variable>

              <xsl:variable name="time">
                <xsl:value-of select="substring-before(@time, ':') * 60 + substring-before(substring-after(@time, ':'), ' ')"/>
              </xsl:variable>

              <xsl:if test="$timesecond != $time">
                <waypoint>
                  <xsl:variable name="name">
                    <xsl:value-of select="@name"/>
                  </xsl:variable>
                  <xsl:if test="xt:node-set($eventmap)/entry[@key = $name]">
                    <alarm>
                      <xsl:value-of select="xt:node-set($eventmap)/entry[@key = $name]"/>
                    </alarm>
                  </xsl:if>

                  <depth>
                    <xsl:call-template name="approximatedepth">
                      <xsl:with-param name="timefirst">
                        <xsl:value-of select="$timefirst"/>
                      </xsl:with-param>
                      <xsl:with-param name="timesecond">
                        <xsl:value-of select="$timesecond"/>
                      </xsl:with-param>
                      <xsl:with-param name="depthfirst">
                        <xsl:value-of select="$depthfirst"/>
                      </xsl:with-param>
                      <xsl:with-param name="depthsecond">
                        <xsl:value-of select="$depthsecond"/>
                      </xsl:with-param>
                      <xsl:with-param name="timeevent">
                        <xsl:call-template name="time2sec">
                          <xsl:with-param name="time">
                            <xsl:value-of select="@time"/>
                          </xsl:with-param>
                        </xsl:call-template>
                      </xsl:with-param>
                    </xsl:call-template>
                  </depth>

                  <divetime><xsl:value-of select="$time"/></divetime>

                  <xsl:if test="@name = 'gaschange'">
                    <xsl:variable name="o2">
                      <xsl:choose>
                        <xsl:when test="@o2 != ''">
                          <xsl:value-of select="number(substring-before(@o2, '%'))"/>
                        </xsl:when>
                        <xsl:otherwise>
                          <xsl:value-of select="21"/>
                        </xsl:otherwise>
                      </xsl:choose>
                    </xsl:variable>

                    <xsl:variable name="he">
                      <xsl:choose>
                        <xsl:when test="@he != ''">
                          <xsl:value-of select="number(substring-before(@he, '%'))"/>
                        </xsl:when>
                        <xsl:otherwise>
                          <xsl:value-of select="0"/>
                        </xsl:otherwise>
                      </xsl:choose>
                    </xsl:variable>

                    <switchmix ref="mix({$o2}/{$he})"/>
                  </xsl:if>

                  <xsl:if test="@name = 'heading'">
                    <heading>
                      <xsl:value-of select="@value"/>
                    </heading>
                  </xsl:if>

                </waypoint>
              </xsl:if>
            </xsl:when>
            <xsl:otherwise>

              <!-- Recorded waypoints and events occurring at the exact same time -->
              <waypoint>

                <xsl:variable name="time">
                  <xsl:value-of select="@time"/>
                </xsl:variable>

                <xsl:for-each select="preceding-sibling::event[@time = $time]">
                  <xsl:variable name="name">
                    <xsl:value-of select="@name"/>
                  </xsl:variable>
                  <xsl:if test="xt:node-set($eventmap)/entry[@key = $name]">
                    <alarm>
                      <xsl:value-of select="xt:node-set($eventmap)/entry[@key = $name]"/>
                    </alarm>
                  </xsl:if>
                </xsl:for-each>

                <depth>
                  <xsl:value-of select="round(substring-before(./@depth, ' ') * 100) div 100"/>
                </depth>

                <divetime>
                  <xsl:call-template name="time2sec">
                    <xsl:with-param name="time">
                      <xsl:value-of select="./@time"/>
                    </xsl:with-param>
                  </xsl:call-template>
                </divetime>

                <xsl:for-each select="preceding-sibling::event[@time = $time and @name='heading']/@value">
                  <heading>
                    <xsl:value-of select="."/>
                  </heading>
                </xsl:for-each>

                <xsl:for-each select="preceding-sibling::event[@time = $time and @name='gaschange']">
                  <xsl:variable name="o2">
                    <xsl:choose>
                      <xsl:when test="@o2 != ''">
                        <xsl:value-of select="number(substring-before(@o2, '%'))"/>
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:value-of select="21"/>
                      </xsl:otherwise>
                    </xsl:choose>
                  </xsl:variable>

                  <xsl:variable name="he">
                    <xsl:choose>
                      <xsl:when test="@he != ''">
                        <xsl:value-of select="number(substring-before(@he, '%'))"/>
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:value-of select="0"/>
                      </xsl:otherwise>
                    </xsl:choose>
                  </xsl:variable>

                  <switchmix ref="mix({$o2}/{$he})"/>
                </xsl:for-each>

                <xsl:if test="./@pressure != ''">
                  <tankpressure>
                    <xsl:value-of select="substring-before(./@pressure, ' ') * 100000"/>
                  </tankpressure>
                </xsl:if>

                <xsl:if test="./@temp != ''">
                  <temperature>
                    <xsl:value-of select="format-number(substring-before(./@temp, ' ') + 273.15, '0.00')"/>
                  </temperature>
                </xsl:if>

                <!-- Recorded waypoints -->
              </waypoint>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
      </samples>

      <informationafterdive>
        <xsl:if test="node()/depth/@max != ''">
          <greatestdepth>
            <xsl:value-of select="substring-before(node()/depth/@max, ' ')"/>
          </greatestdepth>
        </xsl:if>
        <xsl:if test="node()/depth/@mean != ''">
          <averagedepth>
            <xsl:value-of select="substring-before(node()/depth/@mean, ' ')"/>
          </averagedepth>
        </xsl:if>
        <xsl:if test="./@duration != ''">
          <diveduration>
            <xsl:call-template name="time2sec">
              <xsl:with-param name="time">
                <xsl:value-of select="./@duration"/>
              </xsl:with-param>
            </xsl:call-template>
          </diveduration>
        </xsl:if>
        <xsl:if test="temperature/@water|divetemperature/@water != ''">
          <lowesttemperature>
            <xsl:value-of select="format-number(substring-before(temperature/@water|divetemperature/@water, ' ') + 273.15, '0.00')"/>
          </lowesttemperature>
        </xsl:if>
        <notes>
          <para>
            <xsl:value-of select="notes"/>
          </para>
          <xsl:for-each select="picture">
            <link ref="{concat(../@number, ':', @filename, '@', @offset)}"/>
          </xsl:for-each>
        </notes>
        <rating>
          <ratingvalue>
            <xsl:choose>
              <xsl:when test="./@rating = 0">
                <xsl:value-of select="'1'"/>
              </xsl:when>
              <xsl:when test="./@rating != ''">
                <xsl:value-of select="./@rating * 2"/>
              </xsl:when>
            </xsl:choose>
          </ratingvalue>
        </rating>
        <visibility>
            <xsl:choose>
              <xsl:when test="./@visibility = 1">
                <xsl:value-of select="1"/>
              </xsl:when>
              <xsl:when test="./@visibility = 2">
                <xsl:value-of select="3"/>
              </xsl:when>
              <xsl:when test="./@visibility = 3">
                <xsl:value-of select="5"/>
              </xsl:when>
              <xsl:when test="./@visibility = 4">
                <xsl:value-of select="10"/>
              </xsl:when>
              <xsl:when test="./@visibility = 5">
                <xsl:value-of select="15"/>
              </xsl:when>
            </xsl:choose>
        </visibility>
      </informationafterdive>

    </dive>
  </xsl:template>

  <xsl:template match="trip">
      <trip id="trip{generate-id()}"  xmlns="http://www.streit.cc/uddf/3.2/">
        <name><xsl:value-of select="@location"/>&#xA0;<xsl:value-of select="@date"/></name>
        <trippart>
          <name><xsl:value-of select="@location"/>&#xA0;<xsl:value-of select="@date"/></name>
          <relateddives>
            <xsl:for-each select="dive">
              <link ref="{generate-id(.)}"/>
            </xsl:for-each>
          </relateddives>
          <xsl:if test="notes != ''">
            <notes>
              <para>
                  <xsl:value-of select="notes"/>
              </para>
            </notes>
          </xsl:if>
        </trippart>
      </trip>
  </xsl:template>

<!-- Approximate waypoint depth.
     Parameters:
     timefirst    Time of the previous waypoint in seconds
     timesecond   Time of the current waypoint in seconds
     depthfirst   Depth of the first waypoint in mm
     depthsecond  Depth of the second waypoint in mm
     timeevent    Time of the event

     Returns:     Depth approximation of event in m
     -->

<xsl:template name="approximatedepth">
  <xsl:param name="timefirst"/>
  <xsl:param name="timesecond"/>
  <xsl:param name="depthfirst"/>
  <xsl:param name="depthsecond"/>
  <xsl:param name="timeevent"/>

  <xsl:value-of select="format-number((($timeevent - $timefirst) div ($timesecond - $timefirst) * ($depthsecond - $depthfirst) + $depthfirst) div 1000, '#.##')"/>

</xsl:template>

</xsl:stylesheet>
