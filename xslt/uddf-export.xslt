<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:include href="commonTemplates.xsl"/>
  <xsl:strip-space elements="*"/>
  <xsl:output method="xml" encoding="utf-8" indent="yes"/>

  <xsl:key name="gases" match="cylinder" use="concat(substring-before(@o2, '.'), '/', substring-before(@he, '.'))" />

  <xsl:template match="/divelog/dives">
    <uddf version="3.2.0">
      <generator>
        <manufacturer id="subsurface">
          <name>Subsurface Team</name>
          <contact>http://subsurface.hohndel.org/</contact>
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

      <diver>
        <owner id="1">
          <equipment>
            <xsl:for-each select="/divelog/settings/divecomputerid">
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

        <xsl:apply-templates select="//buddy"/>
      </diver>

      <xsl:apply-templates select="//location"/>

      <!-- Define all the unique gases found in the dive log -->
      <gasdefinitions>
        <!-- Get unique gas mixes from all the recorded dives -->
        <xsl:for-each select="dive/cylinder[generate-id() = generate-id(key('gases', concat(substring-before(@o2, '.'), '/', substring-before(@he, '.')))[1])]">

          <xsl:variable name="o2">
            <xsl:choose>
              <xsl:when test="@o2 != ''">
                <xsl:value-of select="substring-before(@o2, '.')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="'21'"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>

          <xsl:variable name="he">
            <xsl:choose>
              <xsl:when test="@he != ''">
                <xsl:value-of select="substring-before(@he, '.')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="'0'"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>

          <!-- The gas definitions will have the o2 percentage as mix ID
               to ease up the reference on switchmix events. Thus we can
               just use the same references used internally on
               Subsurface.
          -->
          <mix id="{$o2}">
            <name>
              <xsl:value-of select="concat($o2, '/', $he)"/>
            </name>
            <o2>
              <xsl:value-of select="$o2"/>
            </o2>
            <he>
              <xsl:value-of select="$he"/>
            </he>
          </mix>
        </xsl:for-each>
      </gasdefinitions>

      <profiledata>
        <xsl:for-each select="trip">
          <repetitiongroup id="{generate-id(.)}">
            <xsl:apply-templates select="dive"/>
          </repetitiongroup>
        </xsl:for-each>
        <xsl:for-each select="dive">
          <repetitiongroup id="{generate-id(.)}">
            <xsl:apply-templates select="."/>
          </repetitiongroup>
        </xsl:for-each>
      </profiledata>
    </uddf>
  </xsl:template>

  <xsl:key name="buddy" match="buddy" use="."/>
  <xsl:template match="buddy">
    <xsl:if test="generate-id() = generate-id(key('buddy', normalize-space(.)))">
      <buddy>
        <xsl:attribute name="id">
          <xsl:value-of select="."/>
        </xsl:attribute>
        <personal>
          <first_name>
            <xsl:value-of select="."/>
          </first_name>
        </personal>
      </buddy>
      </xsl:if>
  </xsl:template>

  <xsl:key name="location" match="location" use="."/>
  <xsl:template match="location">
    <xsl:if test="generate-id() = generate-id(key('location', normalize-space(.)))">
      <dive_site>
        <xsl:attribute name="id">
          <xsl:value-of select="."/>
        </xsl:attribute>
        <name>
          <xsl:value-of select="."/>
        </name>
        <geography>
          <location>
            <xsl:value-of select="."/>
          </location>
          <gps>
            <latitude>
              <xsl:value-of select="substring-before(@gps, ' ')"/>
            </latitude>
            <longitude>
              <xsl:value-of select="substring-after(@gps, ' ')"/>
            </longitude>
          </gps>
        </geography>
      </dive_site>
      </xsl:if>
  </xsl:template>

  <xsl:template match="dive">
    <dive id="{generate-id(.)}">

      <informationbeforedive>
        <xsl:if test="temperature/@air|divetemperature/@air != ''">
          <airtemperature>
            <xsl:value-of select="format-number(substring-before(temperature/@air|divetemperature/@air, ' ') + 273.15, '0.00')"/>
          </airtemperature>
        </xsl:if>
        <datetime>
          <xsl:value-of select="concat(./@date, 'T', ./@time)"/>
        </datetime>
        <divenumber>
          <xsl:value-of select="./@number"/>
        </divenumber>
        <xsl:if test="buddy != ''">
          <buddy_ref>
            <xsl:attribute name="ref">
              <xsl:value-of select="buddy"/>
            </xsl:attribute>
          </buddy_ref>
        </xsl:if>
        <xsl:if test="location != ''">
          <dive_site_ref>
            <xsl:attribute name="ref">
              <xsl:value-of select="location"/>
            </xsl:attribute>
          </dive_site_ref>
        </xsl:if>
      </informationbeforedive>

      <samples>
        <xsl:for-each select="./divecomputer[1]/sample">
          <waypoint>
            <depth>
              <xsl:value-of select="substring-before(./@depth, ' ')"/>
            </depth>
            <divetime>
              <xsl:call-template name="time2sec">
                <xsl:with-param name="time">
                  <xsl:value-of select="./@time"/>
                </xsl:with-param>
              </xsl:call-template>
            </divetime>
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

            <!-- We need to look up if there is an event at the time we
                 are handling currently. And then translate that event
                 to the one in UDDF specification.
            -->
            <xsl:variable name="time">
              <xsl:value-of select="./@time"/>
            </xsl:variable>
            <xsl:if test="preceding-sibling::event/@time = $time">
              <xsl:if test="preceding-sibling::event[@time=$time and @name='gaschange']/@name">

                <!-- Gas change is a reference to the gases section, as
                     the gases index was pure o2 value, we can directly
                     use Subsurfaces reference here.
                -->
                <switchmix>
                  <xsl:attribute name="ref">
                    <xsl:value-of select="preceding-sibling::event[@time=$time and @name='gaschange']/@value"/>
                  </xsl:attribute>
                </switchmix>
              </xsl:if>

              <xsl:if test="preceding-sibling::event[@time=$time and @name='heading']/@name">
                <heading>
                  <xsl:value-of select="preceding-sibling::event[@time=$time and @name='heading']/@value"/>
                </heading>
              </xsl:if>

              <!-- We'll just print the alarm text from our event name
                   as is, deco and surface are specified in UDDF
                   specification but the rest is not recognized and
                   there is no equivalent available.
              -->
              <xsl:if test="preceding-sibling::event[@time=$time and not(@name='heading' or @name='gaschange')]/@name">
                <alarm>
                  <xsl:for-each select="preceding-sibling::event[@time=$time and not(@name='heading' or @name='gaschange')]/@name">
                    <xsl:value-of select="."/>
                  </xsl:for-each>
                </alarm>
              </xsl:if>
            </xsl:if>
          </waypoint>
        </xsl:for-each>
      </samples>

      <tankdata>
        <xsl:if test="cylinder[1]/@size">
          <tankvolume>
            <xsl:value-of select="substring-before(cylinder[1]/@size, ' ')"/>
          </tankvolume>
        </xsl:if>
        <xsl:choose>
          <xsl:when test="divecomputer[1]/sample/@pressure != ''">
            <tankpressurebegin>
              <xsl:value-of select="substring-before(divecomputer[1]/sample/@pressure[1], ' ') * 100000"/>
            </tankpressurebegin>
          </xsl:when>
          <xsl:otherwise>
            <xsl:if test="cylinder[1]/@start">
              <tankpressurebegin>
                <xsl:value-of select="substring-before(cylinder[1]/@start, ' ') * 100000"/>
              </tankpressurebegin>
            </xsl:if>
          </xsl:otherwise>
        </xsl:choose>

        <xsl:choose>
          <xsl:when test="count(divecomputer[1]/sample[@pressure]) &gt; 0">
            <tankpressureend>
              <xsl:value-of select="substring-before(divecomputer[1]/sample[@pressure][last()]/@pressure, ' ') * 100000"/>
            </tankpressureend>
          </xsl:when>
          <xsl:otherwise>
            <xsl:if test="cylinder[1]/@end">
              <tankpressureend>
                <xsl:value-of select="substring-before(cylinder[1]/@end, ' ') * 100000"/>
              </tankpressureend>
            </xsl:if>
          </xsl:otherwise>
        </xsl:choose>

      </tankdata>

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
</xsl:stylesheet>
