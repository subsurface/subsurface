#!/usr/bin/perl

use Data::Dumper;
use JSON;
use Text::CSV;
use utf8;

binmode STDOUT, ":encoding(UTF-8)";

my $csv = Text::CSV->new ( { binary => 1 } )  # should set binary attribute.
                 or die "Cannot use CSV: ".Text::CSV->error_diag ();

open my $fh, "<:encoding(utf8)", $ARGV[0] or die "$ARGV[0]: $!";

@fields = @{$csv->getline($fh)};

$csv->column_names(@fields);


print "<divelog program='Diviac' version='42'>\n<dives>\n";

while(my $dive = $csv->getline_hr($fh)) {
#  print STDERR "Dive number " . $dive->{"Dive #"} . "\n";
  my ($month, $day, $year) = split /\-/, $dive->{"Date"};
  print "<dive number='".$dive->{"Dive #"}."' date='$year-$month-$day' time='".$dive->{"Time in"}.":00' duration='".$dive->{"Duration"}.":00 min'>\n";
  
  print "<depth max='".&fix_feet($dive->{"Max depth"})."' mean='".&fix_feet($dive->{"Avg depth"})."' />\n";
  print "<buddy>" . $dive->{"Dive buddy"} . "</buddy>\n";
  print "<temperature air='" . $dive->{"Surface temp"} . "' water='" . $dive->{"Bottom temp"} . "' />\n";
  print "<location>" . &fix_amp($dive->{"Dive Site"}) .", $dive->{Location}</location>\n";
  print "<gps>$dive->{lat} $dive->{lng}</gps>\n";
  print "<notes>$dive->{Notes}\n\n" . $dive->{"Marine life sighting"} . "\n</notes>\n";
  print "<cylinder size='" . &fix_cuft($dive->{"Tank volume"}, $dive->{"Working pressure"}) . "' start='" . &fix_psi($dive->{"Pressure in"}) ."' end='" . &fix_psi($dive->{"Pressure out"}) . "' description='" . $dive->{"Tank type"} . "' />\n";
  print "<weightsystem weight='" . &fix_lb($dive->{"Weight"})  ."' description='unknown' />";
  print "<divecomputer model='Diviac import'>\n";
  &samples($dive->{"Dive profile data"});
  print "</divecomputer>\n</dive>\n";
}

print "</dives>\n</divelog>\n";

sub samples {
  my $diviac = shift;

#  print STDERR $diviac;
  my $p = eval {decode_json($diviac)};
#  print STDERR Dumper($p);
  my $dive, $events;
  $events = '';
  
  foreach $line (@$p){
    my ($a, $b, $c, $d, $e) = @$line;
    my $min = int($a / 60);
    my $sec = int($a) - 60 * $min;
    my $temp = $c ? "temp = '$c C' " : "";
    $dive .=  "<sample time='$min:$sec min' $temp depth='$b m' />\n";
    
    if (@$d) {
 #     print STDERR "Event at $a: ", (join '|', @$d), "\n";
      my $ev = join(' ', @$d);
      $events .= "<event time ='$min:$sec min' name = '$ev' value='' />\n";
    }
  }
  
  print "$events $dive\n";
}

sub fix_feet {
  my $d = shift;

  if ($d =~ /([\d\.]+)\s*ft/) {
    return ($1 * 0.3048) . ' m';
  } else {
    return $d;
  }
}

sub fix_lb {
  my $d = shift;

  if ($d =~ /([\d\.]+)\s*lb/) {
    return ($1 * 0.453592) . ' kg';
  } else {
    return $d;
  }
}

sub fix_psi {
  my $d = shift;

  if ($d =~ /([\d\.]+)\s*psi/) {
    return ($1 * 0.0689476) . ' bar';
  } else {
    return $d;
  }
}

sub fix_cuft {
  my ($d, $w) = @_;

  my $p;
  
  if ($w =~ /([\d\.]+)\s*psi/) {
    $p = $1 * 0.0689476;
    if ($d =~ /([\d\.]+)\s*ft/) {
      return ($1 * 28.3168 / $p) . ' l';
    } else {
      return $d;
    }
  } else {
    return '';
  }
}

sub fix_amp {
  my $s = shift;
  $s =~ s/\&/\&amp;/g;
  return $s;
}
