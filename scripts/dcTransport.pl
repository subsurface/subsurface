#!/bin/perl
#
# Extract supported divecomputers on Android and iOS from libdivecomputer source
#
# Usage:
#
# dcTransport.pl <path to libdivecomputer/src/descriptor.c> <outfile>
#
use Carp;

#set command line arguments
my ($infi, $outfi) = @ARGV;

if ($infi !~ /.*descriptor.c/) {
	croak "run as $ARGV[0] <path to descriptor.c> <outputfile>\n";
}

open(my $fh, "<", $infi) || croak "can't open $infi: $!";
open(STDOUT, ">", $outfi) || croak "can't open $outfi: $!";

my $ftdi = "\/\/ FTDI";
my $bt =   "\/\/ BT";
my $ble =  "\/\/ BLE";
printf("// This segment of the source is automatically generated\n");
printf("// please edit scripts/dcTransport.pl , regenerated the code and copy it here\n\n");

my @android = ();
my @ios = ();
while (<$fh>) {
	if (/^\s*{\s*"([^\,]*)"\s*,\s*"([^\,]*)"\s*,\s*([^\,]*).*}/) {
		my $v = $1;
		my $p = $2;
		if (/$ftdi/) {
			push(@android, "$v,$p");
		}
		if (/$bt/) {
			push(@android, "$v,$p");
		}
		if (/$ble/) {
			push(@android, "$v,$p");
			push(@ios, "$v,$p");
		}
	}
}

my $lastMod;
my $lastVend;
my @sortedandroid = sort @android;
my @sortedios = sort @ios;
print("#if defined(Q_OS_ANDROID)\n\t/* BT, BLE and FTDI devices */\n");

my $endV;
foreach (@sortedandroid) {
	($vend, $mod) = split(',', $_);
	next if ($vend eq $lastVend && $mod eq $lastMod);
	if ($vend eq $lastVend) {
		printf(", {\"%s\"}", $mod);
	} else {
		printf($endV);
		printf("\tmobileProductList[\"%s\"] =\n\t\tQStringList({{\"%s\"}", $vend, $mod);
		$endV = "});\n";
	}
	$lastVend = $vend;
	$lastMod = $mod;
}
printf($endV);
$endV="";
printf("\n#endif\n#if defined(Q_OS_IOS)\n\t/* BLE only, Qt does not support classic BT on iOS */\n");
foreach (@sortedios) {
	($vend, $mod) = split(',', $_);
	next if ($vend eq $lastVend && $mod eq $lastMod);
	if ($vend eq $lastVend) {
		printf(", {\"%s\"}", $mod);
	} else {
		printf($endV);
		printf("\tmobileProductList[\"%s\"] =\n\t\tQStringList({{\"%s\"}", $vend, $mod);
		$endV = "});\n";
	}
	$lastVend = $vend;
	$lastMod = $mod;
}
printf($endV);
printf("\n#endif\n");
printf("// end of the automatically generated code\n");
close $fh;
