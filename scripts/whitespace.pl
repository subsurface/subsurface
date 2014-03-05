#! /usr/bin/perl

my $input = $ARGV[0];
my $source = `clang-format $input`;
# for_each_dive (...) and Q_FOREACH and friends...
$source =~ s/^(.*each.*\(.*\).*)\n\s*{\s*$/$1 {/img;
$source =~ s/(?:\G|^)(\s+[^#\/*].*each.*\(.*\))\n(\s*)([^{}\s])/$1\n\t$2$3/img;

$source =~ s/^(\s*struct[^()\n]*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\s*static\s+struct[^()\n]*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\s*union[^()\n]*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\s*static\s+union[^()\n]*)\n\s*{\s*$/$1 {/img;
$source =~ s/^(\s*class.*)\n\s*{\s*$/$1 {/img;
# colon goes at the end of a line
$source =~ s/^(\S*::\S*.*)\n\s*: /$1 : /img;
# odd indentations from flang-format:
# six spaces or four spaces after tabs (for continuation strings)
$source =~ s/(?:\G|^)[ ]{6}/\t/mg;
$source =~ s/(?:\G|^)(\t*)[ ]{4}"/$1\t"/mg;
# the next ones are rather awkward
# they capture multi line #define and #if definded statements
# that clang-format messes up (where does that 4 space indentation come
# from?
# I couldn't figure out how to make it apply to an arbitrary number of
# intermediate lines, so I hardcoded 0 through 5 lines between the #define
# or #id defined statements and the end of the multi line statement
$source =~ s/^(#(?:if |)define.*)\n +([^*].*)$/$1\n\t$2/mg;
$source =~ s/^(#(?:if |)define.*)((?:\\\n.*){1})\n +([^*].*)$/$1$2\n\t$3/mg;
$source =~ s/^(#(?:if |)define.*)((?:\\\n.*){2})\n +([^*].*)$/$1$2\n\t$3/mg;
$source =~ s/^(#(?:if |)define.*)((?:\\\n.*){3})\n +([^*].*)$/$1$2\n\t$3/mg;
$source =~ s/^(#(?:if |)define.*)((?:\\\n.*){4})\n +([^*].*)$/$1$2\n\t$3/mg;
$source =~ s/^(#(?:if |)define.*)((?:\\\n.*){5})\n +([^*].*)$/$1$2\n\t$3/mg;
# don't put line break before the last single term argument of a
# calculation
$source =~ s/(?:\G|^)(.*[+-])\n\s*(\S*\;)$/$1 $2/mg;
open (DIFF, "| diff -u $input -");
print DIFF $source ;
