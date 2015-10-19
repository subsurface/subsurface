#!/usr/bin/perl -CS

use strict;
use utf8;
use XML::TreeBuilder;

foreach my $file_name (@ARGV) {
	my $tree = XML::TreeBuilder->new({'NoExpand' => 0, 'ErrorContext' => 0});
	$tree->parse_file($file_name, ProtocolEncoding => 'UTF-8');
	foreach my $string ($tree->find_by_tag_name('message')) {
		my $source = $string->find_by_tag_name('source')->as_text;
		my $translation = $string->find_by_tag_name('translation')->as_text;
		next unless $translation =~ /\S/;
		my @source_args = ($source =~ /\%([^\s\-\(\)])/g);
		my @translation_args = ($translation =~ /\%([^\s\-\(\)])/g);
		if (scalar(@source_args) != scalar(@translation_args)) {
			print "$file_name:\n$source\n->\n$translation\n\n";
		}
	}
}
