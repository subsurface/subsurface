#! /usr/bin/perl

my $input = $ARGV[0];
my $source = `clang-format $input`;
$source =~ s/^(.*each.*\(.*\).*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(.*struct.*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(.*class.*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\S*::\S*.*)\n\s*: /$1 :\n\t/img;
$source =~ s/(?:\G|^)[ ]{6}/\t/mg;
open (DIFF, "| diff -u $input -");
print DIFF $source ;
