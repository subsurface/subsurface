#!/usr/bin/env perl

use CGI;

$CGI::POST_MAX = 1024 * 1024 * 10;

# Change this to the correct path to binary.
my $smtk2ssrf = "../build/smtk2ssrf";
my $diviac = "../scripts/diviac.pl";
my $logfile = '/tmp/smtk2ssrf.log';

my $q = CGI->new;

if ($q->upload("uploaded_file")) {
        my $original_filename = $q->param("uploaded_file");
        my $converted = `$smtk2ssrf $tmp_filename -`;
        my $tmp_filename = $q->tmpFileName($original_filename);
        my $new_filename = $original_filename;
        $new_filename =~ s/.*[\/\\]//;
        $new_filename =~ s/\..*$/.ssrf/;
	my $converted;
	if ($q->param('filetype') eq "Diviac") {
		$converted = `$diviac $tmp_filename`;
	} else {
		$converted = `$smtk2ssrf $tmp_filename -`;
	}

	if (length($converted) > 5) {

	        print "Content-Disposition: attachment; filename=\"$new_filename\"\n";
		print "Content-type: subsurface/xml\n\n";
		print $converted;
	} else {
		print "Content-type: text/html\n\n";
		print "<H1>Conversion failed</H1>";
		print 'Please contact <a href=mailto:helling@atdotde.de>Robert Helling (robert@thetheoreticaldiver.org)</a> if the problem persits.';
	}
} else {
        print "Content-type: text/html\n\n";

# Here we could print some header stuff to fit better in the subsurface webspace context. Do do so uncomment
# open (IN, "filename_header.html);
# while (<IN>) {
#         print;
# }
# close(IN);

        print $q->start_multipart_form();

        print $q->h1("Convert Smartrack and Diviac files to Subsurface");
        print $q->filefield( -name => "uploaded_file",
                             -size => 50,
                             -maxlength => 200);
	print $q->popup_menu(-name => "filetype", -values => ["Smartrack", "Diviac"]);
        print $q->submit();
        print $q->end_form();

# Here we could print some footer stuff as above

}
