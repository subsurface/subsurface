#!/usr/bin/perl -CS

# Run this script to check if translations messed up format string arguments
#
# Usage  scripts/checktranslations.pl translations/*ts

use strict;
use utf8::all;
use XML::TreeBuilder;

foreach my $file_name (@ARGV) {
	my $tree = XML::TreeBuilder->new({'NoExpand' => 0, 'ErrorContext' => 0});
	$tree->parse_file($file_name, ProtocolEncoding => 'UTF-8');
	foreach my $string ($tree->find_by_tag_name('message')) {
		my $source = $string->find_by_tag_name('source')->as_text;
		my $translation = $string->find_by_tag_name('translation');
		if ($translation->find_by_tag_name('numerusform')) {
			my @all = $translation->find_by_tag_name('numerusform');
			foreach my $transstring(@all) {
				&compare($file_name, $source, $transstring->as_text);
			}
		} else {
			&compare($file_name, $source, $translation->as_text);
		}
	}
}

sub compare {
	my ($file_name, $src, $trans) = @_;

#	print "Checking $src\n";
       	return unless $trans =~ /\S/;
	my @source_args = sort {$a cmp $b} ($src =~ /\%([^\s\-\(\)\,\.\:])/g);
        my @translation_args = sort {$a cmp $b} ($trans =~ /\%([^\s\-\(\)\,\.\:])/g);
#	print join("|", @translation_args), " vs ", join("|", @source_args),"\n";
        unless (join('', @source_args) eq join('', @translation_args)) {
        	print "$file_name:\n$src\n->\n$trans\n\n";
        }
}
 
