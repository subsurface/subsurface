#!/usr/bin/env perl

# This script expects filenames on the command line and looks up text: tags in qml files and tries to wrap them with qsTr

foreach $filename (@ARGV) {
  next unless $filename =~ /qml$/;
  open IN, $filename;
  open OUT, ">$filename.bak";
  while (<IN>) {
    if (/text:/ && !/qsTr/) {
      if (/text:\s+(\"[^\"]*\")\s*$/) {
	print OUT "$`text: qsTr($1)$'\n";
      } else {
	print OUT ">>>>$_";
	print "$filename: $_";
      }
    } else {
      print OUT;
    }
  }
  close IN;
  close OUT;
  system "mv $filename.bak $filename";
}
