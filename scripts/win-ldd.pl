#!perl
use strict;
my %deploy;
my $objdump = $ENV{objdump} ? $ENV{objdump} : "objdump";
my @searchdirs;
my @systemdirs;
if ($ENV{PATH} =~ "x86_64") {
	@systemdirs = (qr|^c:/windows|i, qr|^c:/winnt|i, qr|^/c/windows|i, qr|^/c/winnt|, qr|^/usr/i686|);
} else {
	@systemdirs = (qr|^c:/windows|i, qr|^c:/winnt|i, qr|^/c/windows|i, qr|^/c/winnt|);
}

sub addDependenciesFor($) {
	open OBJDUMP, "-|", $objdump, "-p", $_[0] or die;
	while (<OBJDUMP>) {
		last if /^The Import Tables/;
	}
	while (<OBJDUMP>) {
		next unless /DLL Name: (.*)/;
		$deploy{$1} = 0 unless defined($deploy{$1});
		last if /^\w/;
	}
	close OBJDUMP;
}

sub findMissingDependencies {
	for my $name (keys %deploy) {
		next if $deploy{$name};
		my $path;
		for my $dir (@searchdirs) {
			my $fpath = "$dir/$name";
			my $lcfpath = "$dir/" . lc($name);
			if (-e $fpath) {
				$path = $fpath;
			} elsif (-e $lcfpath) {
				$path = $lcfpath;
			} else {
				next;
			}
			addDependenciesFor($path);
			last;
		}

		$path = "/missing/file" unless $path;
		$deploy{$name} = $path;
	}
}

for (@ARGV) {
	s/^-L//;
	next if /^-/;
	if (-d $_) {
		push @searchdirs, $_;
	} elsif (-f $_ && ! /\.a$/) {
		# Add $_'s path to the search list too
		my $dirname = $_;
		$dirname =~ s,/[^/]+$,,;
		unshift @searchdirs, $dirname;

		$deploy{$_} = $_;
		addDependenciesFor($_);
	}
}

# Append PATH to @searchdirs
@searchdirs = (@searchdirs, split(/:/, $ENV{PATH}));

# Remove system dirs from @searchdirs
@searchdirs = grep {
	my $sys = 0;
	for my $rx (@systemdirs) {
		if ($_ =~ $rx) {
			$sys = 1;
			last;
		}
	}
	!$sys;
} @searchdirs;

while (1) {
	findMissingDependencies();

	my $i = 0;
	while (my ($name, $path) = each(%deploy)) {
		next if $path;
		++$i;
		last;
	}
	last if $i == 0;
}

for (sort values %deploy) {
	next if $_ eq "/missing/file";
	print "$_\n";
}
