use Carp;

#set command line arguments
my ($infi, $outfi) = @ARGV;
my ($type) = $outfi =~ /\.([^.]+)$/;

open(my $fh, "<", $infi) || croak "can't open $infi: $!";
open(STDOUT, ">", $outfi) || croak "can't open $outfi: $!";

my $lastVend = "";
while (<$fh>) {
    my ($vend, $mod, $set) = split('\t', $_);
    if ($type eq "html") {
	if ($vend eq $lastVend) {
	    printf(", %s", $mod);
	} else {
	    if ($lastVend eq "") {
		printf("<ul><li>%s\n\t<ul>\n\t    <li>%s", $vend, $mod);
	    } else {
		printf("</li>\n\t</ul>\n    </li>\n    <li>%s\n\t<ul>\n\t    <li>%s", $vend, $mod);
	    }
	}
    } else {
	if ($vend eq $lastVend) {
	    printf(", %s", $mod);
	} else {
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
    print("</li>\n\t</ul>\n    </li>\n<ul>");
}
close $fh;
