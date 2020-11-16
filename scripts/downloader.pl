#!/usr/bin/perl

use CGI;

my $q = CGI->new;

print $q->header('text/html');
print $q->img({src => 'https://subsurface-divelog.org/wp-content/uploads/2015/10/subsurface-icon1.png'});
print $q->h1("Subsurface");

my %dcs;
&load_supported_dcs;

print $q->start_form();
if ($q->param("Manufacturer")) {
  print $q->hidden(-name => "Manufacturer", -default => $q->param("Manufacturer"));
  if ($q->param("Product")) {
    print $q->hidden(-name => "Product", -default => $q->param("Product"));
    opendir DIR, "/dev";
    my @devices = map {"/dev/$_"} (grep {!/^\./} (readdir DIR));
    closedir DIR;
    print "Select mount point:";
    print $q->popup_menu("Mount point", \@devices);
  } else {
    print "Select ",$q->param("Manufacturer")," model:";
    print $q->popup_menu("Product", $dcs{$q->param("Manufacturer")});
  }
} else {
  print "Select dive computer manufacturer:";
  print $q->popup_menu("Manufacturer", [sort keys %dcs]);
}

print $q->submit();

print $q->end_form();

sub load_supported_dcs {
  open IN, "/home/pi/src/subsurface/build/subsurface-downloader --list-dc|";
  
  while(<IN>) {
    last if /Supported dive computers:/;
  }
  while(<IN>) {
    last unless /\S/;
    my ($manufacturer, $products) = /"([^:]+):\s+([^"]+)"/;
    
    $products =~ s/\([^\)]*\)//g;
    my @products  = split /,\s*/, $products;
    $dcs{$manufacturer} = \@products;
  }
  close IN;
}

