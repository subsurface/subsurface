<?xml version="1.0" encoding="iso-8859-1"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:include href="commonTemplates.xsl" />
  <xsl:strip-space elements="*" />
  <xsl:param name="allcylinders" select="allcylinders" />
  <xsl:output method="xml" encoding="utf-8" indent="yes" omit-xml-declaration="yes"
    cdata-section-elements="location divesite weather visibility partner boatname cylinderdescription lognotes"
  />

  <xsl:template match="/divelog/dives">
    <xsl:apply-templates select="dive" />
  </xsl:template>

  <xsl:template match="dive">
    <dive ignore-root="yes">
      <date>
        <xsl:value-of select="@date" />
      </date>
      <time>
        <xsl:value-of select="@time" />
      </time>
      <xsl:if test="divecomputer/@model != ''">
        <dc_model>
          <xsl:value-of select="divecomputer/@model" />
        </dc_model>
      </xsl:if>

      <xsl:variable name="duration">
        <xsl:call-template name="time2sec">
          <xsl:with-param name="time">
            <xsl:value-of select="@duration" />
          </xsl:with-param>
        </xsl:call-template>
      </xsl:variable>

      <duration>
        <xsl:value-of select="$duration" />
      </duration>

      <xsl:variable name="uuid">
        <xsl:value-of select="@divesiteid" />
      </xsl:variable>
      <xsl:variable name="location">
        <xsl:value-of select="//site[@uuid = $uuid]/@name" />
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="/divelog/divesites/site[@uuid = $uuid]/geo/@value != ''">
          <location>
            <xsl:for-each select="/divelog/divesites/site[@uuid = $uuid]/geo/@value">
              <xsl:if test="position() != 1"> / </xsl:if>
            <xsl:value-of select="." />
            </xsl:for-each>
          </location>
        <divesite>
            <xsl:value-of select="$location" />
          </divesite>
        </xsl:when>
        <xsl:when test="contains($location, '/')">
          <xsl:variable name="site">
            <xsl:call-template name="basename">
              <xsl:with-param name="value" select="$location" />
            </xsl:call-template>
          </xsl:variable>
        <divesite>
            <xsl:value-of select="$site" />
          </divesite>
        <location>
            <xsl:value-of
              select="substring($location, 0, string-length($location) - string-length($site))" />
          </location>
        </xsl:when>
        <xsl:otherwise>
          <divesite>
            <xsl:value-of select="$location" />
          </divesite>
        </xsl:otherwise>
      </xsl:choose>

      <visibility>
        <xsl:value-of select="@visibility" />
      </visibility>
      <buddy>
        <xsl:value-of select="buddy" />
      </buddy>

      <!-- If there is a gas change event within the first few seconds
         then we try to detect matching cylinder, otherwise the first
         cylinder is used.
         -->
      <xsl:variable name="time">
        <xsl:call-template name="time2sec">
          <xsl:with-param name="time">
            <xsl:value-of select="event[@name = 'gaschange']/@time" />
          </xsl:with-param>
        </xsl:call-template>
      </xsl:variable>

      <xsl:variable name="mix">
        <xsl:value-of select="concat(event[@name = 'gaschange']/@value, '.0%')" />
      </xsl:variable>

      <xsl:variable name="cylinder">
        <xsl:choose>
          <xsl:when test="$time &lt; 60">
            <xsl:value-of select="count(cylinder[@o2 = $mix]/preceding-sibling::cylinder) + 1" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="1" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <tanks type="array">
        <xsl:for-each select="cylinder">
          <tank>
            <tank>
              <xsl:value-of select="@description" />
            </tank>
            <tankname>
              <xsl:value-of select="@description" />
            </tankname>

            <xsl:variable name="double">
              <xsl:choose>
                <xsl:when test="substring(@description, 1, 1) = 'D' and substring-before(substring(@description, 2), ' ') * 2 = substring-before(@size, ' ')">
                  <xsl:value-of select="'2'" />
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="'1'" />
                </xsl:otherwise>
              </xsl:choose>
            </xsl:variable>
            <dbltank>
              <xsl:value-of select="$double - 1" />
            </dbltank>
            <vol>
              <xsl:value-of select="substring-before(@size, ' ') div $double" />
            </vol>
            <start_pressure>
              <xsl:choose>
                <xsl:when test="node()/sample/@pressure != ''">
                  <xsl:value-of select="substring-before(node()/sample/@pressure, ' ')" />
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="substring-before(@start, ' ')" />
                </xsl:otherwise>
              </xsl:choose>
            </start_pressure>
            <end_pressure>
              <xsl:choose>
                <xsl:when test="count(node()/sample[@pressure!='']) &gt; 0">
                  <xsl:value-of select="node()/sample[@pressure][last()]/@pressure" />
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="substring-before(@end, ' ')" />
                </xsl:otherwise>
              </xsl:choose>
            </end_pressure>
            <xsl:choose>
              <xsl:when test="@workpressure != ''">
                <wp>
                  <xsl:value-of select="substring-before(@workpressure, ' ')" />
                </wp>
              </xsl:when>
              <xsl:otherwise>
                <wp>198</wp>
              </xsl:otherwise>
            </xsl:choose>
            <o2>
              <xsl:choose>
                <xsl:when test="@o2 != ''">
                  <xsl:value-of select="substring-before(@o2, '%')" />
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="'21'" />
                </xsl:otherwise>
              </xsl:choose>
            </o2>
            <he>
              <xsl:value-of select="substring-before(@he, '%')" />
            </he>
          </tank>
        </xsl:for-each>
      </tanks>

      <weights>
        <xsl:call-template name="sum">
          <xsl:with-param name="values" select="weightsystem/@weight" />
        </xsl:call-template>
      </weights>
      <notes>
        <xsl:value-of select="notes" />
      </notes>
      <lat>
        <xsl:value-of select="substring-before(//site[@uuid = $uuid]/@gps, ' ')" />
      </lat>
      <lng>
        <xsl:value-of select="substring-after(//site[@uuid = $uuid]/@gps, ' ')" />
      </lng>
      <maxdepth>
        <xsl:value-of select="substring-before(node()/depth/@max, ' ')" />
      </maxdepth>
      <meandepth>
        <xsl:value-of select="substring-before(node()/depth/@mean, ' ')" />
      </meandepth>
      <airtemp>
        <xsl:value-of select="substring-before(node()/temperature/@air|divetemperature/@air, ' ')" />
      </airtemp>
      <depthtemp>
        <xsl:value-of select="substring-before(node()/temperature/@water|divetemperature/@water, ' ')" />
      </depthtemp>
      <xsl:variable name="manual">
        <xsl:choose>
          <xsl:when test="divecomputer/@model = 'manually added dive'">
            <xsl:value-of select="1" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="0" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>

      <!-- Test if dive computer requires special handling -->
      <xsl:variable name="special">
        <xsl:choose>
          <xsl:when test="divecomputer/@model = 'Suunto EON Steel'">
            <xsl:text>1</xsl:text>
          </xsl:when>
        </xsl:choose>
      </xsl:variable>

      <samplerate>
        <xsl:choose>
          <xsl:when test="$special = 1">
            <xsl:variable name="samples">
              <xsl:value-of select="count(//sample/@time)" />
            </xsl:variable>
          <xsl:value-of select="ceiling($duration div $samples)" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:variable name="first">
              <xsl:call-template name="time2sec">
                <xsl:with-param name="time">
                  <xsl:value-of select="node()/sample[1]/@time" />
                </xsl:with-param>
              </xsl:call-template>
            </xsl:variable>
          <xsl:variable name="second">
              <xsl:call-template name="time2sec">
                <xsl:with-param name="time">
                  <xsl:value-of select="node()/sample[2]/@time" />
                </xsl:with-param>
              </xsl:call-template>
            </xsl:variable>
          <xsl:choose>
              <xsl:when test="$manual = 0">
                <xsl:value-of select="$second - $first" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="60" />
              </xsl:otherwise>
            </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </samplerate>
      <sampledata type="array">
        <xsl:for-each select="divecomputer[1]/sample">
          <xsl:choose>
            <xsl:when test="$manual = 1 and @time != '0:00 min'">
              <xsl:variable name="timesecond">
                <xsl:call-template name="time2sec">
                  <xsl:with-param name="time" select="@time" />
                </xsl:call-template>
              </xsl:variable>
              <xsl:variable name="timefirst">
                <xsl:call-template name="time2sec">
                  <xsl:with-param name="time" select="preceding-sibling::sample[1]/@time" />
                </xsl:call-template>
              </xsl:variable>
              <xsl:call-template name="while">
                <xsl:with-param name="until">
                  <xsl:value-of select="($timesecond - $timefirst) div 60 - 1" />
                </xsl:with-param>
                <xsl:with-param name="count">
                  <xsl:value-of select="0" />
                </xsl:with-param>
                <xsl:with-param name="timefirst">
                  <xsl:value-of select="$timefirst" />
                </xsl:with-param>
                <xsl:with-param name="timesecond">
                  <xsl:value-of select="$timesecond" />
                </xsl:with-param>
                <xsl:with-param name="depthsecond">
                  <xsl:call-template name="depth2mm">
                    <xsl:with-param name="depth">
                      <xsl:value-of select="./@depth" />
                    </xsl:with-param>
                  </xsl:call-template>
                </xsl:with-param>
                <xsl:with-param name="depthfirst">
                  <xsl:call-template name="depth2mm">
                    <xsl:with-param name="depth">
                      <xsl:value-of select="preceding-sibling::sample[1]/@depth" />
                    </xsl:with-param>
                  </xsl:call-template>
                </xsl:with-param>
              </xsl:call-template>
              <!--  <name select="{concat(@time, ' - ', preceding-sibling::sample[1]/@time)}"/>-->
            </xsl:when>
            <xsl:otherwise>
              <xsl:choose>
                <!-- Suunto EON Steel special case -->
                <xsl:when test="$special = 1">
                  <xsl:variable name="cur">
                    <xsl:call-template name="time2sec">
                      <xsl:with-param name="time">
                        <xsl:value-of select="@time" />
                      </xsl:with-param>
                    </xsl:call-template>
                  </xsl:variable>
                  <xsl:variable name="prev">
                        <xsl:call-template name="time2sec">
                          <xsl:with-param name="time">
                            <xsl:value-of select="preceding-sibling::sample[1]/@time" />
                          </xsl:with-param>
                        </xsl:call-template>
                      </xsl:variable>
                  <xsl:variable name="prevprev">
                    <xsl:call-template name="time2sec">
                      <xsl:with-param name="time">
                        <xsl:value-of select="preceding-sibling::sample[2]/@time" />
                      </xsl:with-param>
                    </xsl:call-template>
                  </xsl:variable>
                  <xsl:choose>
                    <xsl:when test="$cur - $prev &gt;= 10 or $prev = 0 or $prevprev = 0">
                      <SAMPLE>
                        <xsl:choose>
                          <xsl:when test="./@temp != ''">
                            <d>
                              <xsl:value-of select="substring-before(./@depth, ' ')" />
                            </d>
                            <t>
                              <xsl:value-of select="substring-before(./@temp, ' ')" />
                            </t>
                          </xsl:when>
                          <xsl:otherwise>
                            <xsl:value-of select="substring-before(./@depth, ' ')" />
                          </xsl:otherwise>
                        </xsl:choose>
                      </SAMPLE>
                    </xsl:when>
                  </xsl:choose>
                </xsl:when>
                <xsl:otherwise>
                  <SAMPLE>
                    <xsl:choose>
                      <xsl:when test="./@temp != ''">
                        <d>
                          <xsl:value-of select="substring-before(./@depth, ' ')" />
                        </d>
                        <t>
                          <xsl:value-of select="substring-before(./@temp, ' ')" />
                        </t>
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:value-of select="substring-before(./@depth, ' ')" />
                      </xsl:otherwise>
                    </xsl:choose>
                  </SAMPLE>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
      </sampledata>
    </dive>
  </xsl:template>

  <xsl:template name="while">
    <xsl:param name="timefirst" />
    <xsl:param name="timesecond" />
    <xsl:param name="depthfirst" />
    <xsl:param name="depthsecond" />
    <xsl:param name="count" />
    <xsl:param name="until" />

    <xsl:variable name="curdepth">
      <xsl:value-of select="format-number(((($timefirst + 60) - $timefirst) div ($timesecond - $timefirst) * ($depthsecond - $depthfirst) + $depthfirst), '#.##')" />
    </xsl:variable>
    <xsl:variable name="curtime">
      <xsl:value-of select="$timefirst + 60" />
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$count &lt; $until">
        <SAMPLE>
          <xsl:value-of select="$curdepth div 1000" />
        </SAMPLE>
        <xsl:call-template name="while">
          <xsl:with-param name="timefirst" select="$curtime" />
          <xsl:with-param name="timesecond" select="$timesecond" />
          <xsl:with-param name="depthfirst" select="$curdepth" />
          <xsl:with-param name="depthsecond" select="$depthsecond" />
          <xsl:with-param name="count" select="$count + 1" />
          <xsl:with-param name="until" select="$until" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="$curdepth &gt; 0">
          <SAMPLE>
            <xsl:value-of select="$curdepth div 1000" />
          </SAMPLE>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>

  </xsl:template>

  <xsl:template name="basename">
    <xsl:param name="value" />

    <xsl:choose>
      <xsl:when test="contains($value, '/')">
        <xsl:call-template name="basename">
          <xsl:with-param name="value" select="substring-after($value, '/')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$value" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
