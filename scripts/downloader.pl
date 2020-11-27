#!/usr/bin/perl -w

use strict;

use CGI;
use Git::Repository;

# File to store cloud credentials
my $config_file = "/opt/ssrf/ssrf.conf";
# Where to store the git repository
my $git_dir = "/opt/ssrf/gitdir";
# Downloader binary
my $downloader = "/home/pi/src/subsurface/build/subsurface-downloader";

my %conf;

# Use unbuffered output
$| = 1;

my $q = CGI->new;

print $q->header('text/html');
print $q->img({src => 'https://subsurface-divelog.org/wp-content/uploads/2015/10/subsurface-icon1.png'});
print $q->h1("Subsurface");

printf "Reading config file $config_file\n";
open CONF, $config_file || die "Cannot read $config_file:$!";
while (<CONF>) {
  if (/^\s*(\w+)\s*=\s*(\w.*)$/) {
    $conf{$1} = $2;
  }
}
close CONF;

my %dcs;
&load_supported_dcs;

print $q->start_form();

my $action = $q->param("action");

if ($action eq "config") {

  # Enter cloud credentials

  print "Subsurface cloud user name (typically your email address): ", $q->textfield(-name => 'username', -default => $conf{username});
  print "<br>Subsurface cloud password: ", $q->password_field(-name => "password");
  &next_action("writeconfig");

} elsif ($action eq "writeconfig") {

  $conf{username} = $q->param("username");
  $conf{username} =~ s/\s//g;
  $conf{password} = $q->param("password");
  $conf{password} =~ s/\s//g;
  &write_conf;
  &next_action("start");

} elsif ($action eq "setmanufacturer") {

  # Now we know the manufacturer, ask for model

  print $q->hidden(-name => "Manufacturer", -default => $q->param("Manufacturer"));
  print "Select ",$q->param("Manufacturer")," model:";
  print $q->popup_menu("Product", $dcs{$q->param("Manufacturer")});
  &next_action("setproduct")
    
} elsif ($action eq "setproduct") {

  # Now we know the model as well, ask for mount point

  print $q->hidden(-name => "Manufacturer", -default => $q->param("Manufacturer"));
  print $q->hidden(-name => "Product", -default => $q->param("Product"));

  opendir DIR, "/dev";
  my @devices = map {"/dev/$_"} (grep {!/^\./} (readdir DIR));
  closedir DIR;
  print "Select mount point:";
  print $q->popup_menu(-name => "Mount point", -values => \@devices);
  &next_action("startdownload");

} elsif ($action eq "startdownload") {

  # Do the actual download

  my $repo;

  # Does the repo exist?

  if (-d $git_dir) {

    # ... yes, pull latest version

    $repo = Git::Repository->new( work_tree => $git_dir);
    print "Pulling latest version from cloud.";
    print $q->pre($repo->run("pull"));
  } else {

    # ... no, clone it

    my $en_username = $conf{username};

    # We need to escape the @ in the username to be able to encode it in the URL.
    # Note: If we fail, the password gets written to /var/log/apache/error.log,
    # Maybe there is a better way to pass the password to git...
    
    $en_username =~ s/\@/%40/g;
    my $git_url = 'https://' . $en_username . ':' . $conf{password} . '@cloud.subsurface-divelog.org//git/' . $conf{username};
    print "Cloning repository";
    print $q->pre(Git::Repository->run( clone => $git_url, $git_dir));
    $repo = Git::Repository->new( work_tree => $git_dir );
  }

  # Assemble the command with all arguments
  
  my $command = "$downloader --dc-vendor=" . $q->param('Manufacturer') .
		" --dc-product=" . $q->param('Product') .
		" --device=" . $q->param("Mount point") .
		' ' . $git_dir .
		'/[' . $conf{username} . ']';
  print $q->pre($command);

  # ... and run it
  
  print $q->pre(`$command`);

  # Push back to the cloud
  
  print "Checkout user branch";
  print $q->pre($repo->run("checkout", $conf{username}));
  print "Push changes to cloud";
  print $q->pre($repo->run("push", "origin", $conf{username}));
  &next_action("start");
} else {

  # This is the mode we start up in
  
  print "Select dive computer manufacturer:";
  print $q->popup_menu("Manufacturer", [sort keys %dcs]);
  &next_action("setmanufacturer")
}


print $q->br(),$q->submit(-name => "  OK  ");
print $q->end_form();

print $q->br(), $q->a({-href => $q->url() . "?action=config"}, "Configure cloud credentials");

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


sub write_conf {
  print "Writing config file\n";
  open CONFW, ">$config_file" || die "Cannot write $config_file:$!";
  foreach my $key (keys %conf) {
    print CONFW "$key = $conf{$key}\n";
  }
  close CONFW;
  print "Done\n";
}

sub next_action {
  my $next = shift;
  $q->param(action => $next);
  print $q->hidden(
		   -name => "action",
		   -value => $next);
  return;
}
