#!/bin/perl
#
# Extract supported divecomputers from libdivecomputer source
#
# Usage:
#
# parse-descriptor.pl <path to libdivecomputer/src/descriptor.c> <outfile>
#
# depending on suffix of the outfile it creates the right content for
# either a text file or and html file
use Carp;

#set command line arguments
my ($infi, $outfi) = @ARGV;
my ($type) = $outfi =~ /\.([^.]+)$/;

if ($infi !~ /.*descriptor.c/) {
	croak "run as $ARGV[0] <path to descriptor.c> <outputfile>\n";
}

open(my $fh, "<", $infi) || croak "can't open $infi: $!";
open(STDOUT, ">", $outfi) || croak "can't open $outfi: $!";

my $lastVend = "";
my @descriptors = ();
while (<$fh>) {
	if (/^\s*{\s*"([^\,]*)"\s*,\s*"([^\,]*)"\s*,\s*([^\,]*).*}/) {
		push(@descriptors, "$1,$2");
	}
}
my @sortedDescriptors = sort @descriptors;
foreach (@sortedDescriptors) {
	($vend, $mod) = split(',', $_);
	if ($type eq "html") {
		if ($vend eq $lastVend) {
			printf(", %s", $mod);
		} else {
			if ($lastVend lt "Uemis" && $vend gt "Uemis") {
				printf("</li></ul>\n    </dd>\n    <dt>Uemis</dt><dd><ul>\n\t    <li>Zürich SDA");
			}
			if ($lastVend eq "") {
				printf("<dl><dt>%s</dt><dd><ul>\n\t    <li>%s", $vend, $mod);
			} else {
				printf("</li></ul>\n    </dd>\n    <dt>%s</dt><dd><ul>\n\t    <li>%s", $vend, $mod);
			}
		}
	} else {
		if ($vend eq $lastVend) {
			printf(", %s", $mod);
		} else {
			if ($lastVend lt "Uemis" && $vend gt "Uemis") {
				printf("\nUemis: Zürich SDA");
			}
			if ($lastVend eq "") {
				printf("%s: %s", $vend, $mod);
			} else {
				printf("\n%s: %s", $vend, $mod);
			}
		}
	}
	$lastVend = $vend;
}
if ($type eq "html") {
    print("</li>\n\t</ul>\n    </dd>\n</dl>");
}
close $fh;
