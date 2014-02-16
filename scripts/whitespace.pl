#! /usr/bin/perl

my $input = $ARGV[0];
my $source = `clang-format $input`;
$source =~ s/^(.*each.*\(.*\).*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\s*struct[^()\n]*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\s*static\s+struct[^()\n]*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\s*union[^()\n]*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\s*static\s+union[^()\n]*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\s*class.*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\S*::\S*.*)\n\s*: /$1 : /img;
$source =~ s/(?:\G|^)[ ]{6}/\t/mg;
$source =~ s/(?:\G|^)(\t*)[ ]{4}"/$1\t"/mg;
open (DIFF, "| diff -u $input -");
print DIFF $source ;
