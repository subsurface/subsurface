#!/usr/bin/env perl

use CGI;

# Change this to the correct path to binary.
my $smtk2ssrf = "../build/smtk2ssrf";

my $q = CGI->new;

if ($q->upload("uploaded_file")) {
        my $original_filename = $q->param("uploaded_file");
        my $tmp_filename = $q->tmpFileName($original_filename);
        my $new_filename = $original_filename;
        $new_filename =~ s/.*[\/\\]//;
        $new_filename =~ s/\..*$/.ssrf/;

        print "Content-Disposition: attachment; filename=\"$new_filename\"\n";
        print "Content-type: subsurface/xml\n\n";
        system "$smtk2ssrf $tmp_filename -";
} else {
        print "Content-type: text/html\n\n";

# Here we could print some header stuff to fit better in the subsurface webspace context. Do do so uncomment
# open (IN, "filename_header.html);
# while (<IN>) {
#         print;
# }
# close(IN);

        print $q->start_multipart_form();

        print $q->h1("Convert Smartrack files to Subsurface");

        print $q->filefield( -name => "uploaded_file",
                             -size => 50,
                             -maxlength => 200);
        print $q->submit();
        print $q-end_form();

# Here we could print some footer stuff as above

}
