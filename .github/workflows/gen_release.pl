#!/usr/bin/perl

my $body_path = shift or die "Need a file name to store the release body";

my $ver;

open IN, "configure.ac" or die;
while (<IN>) {
	if (m/^[^\#]*AC_INIT\s*\(\s*\[\s*zbar\s*\]\s*,\s*\[(\d+[\.\d]+)/) {
		$ver=$1;
		last;
	}
}
close IN or die;

die if (!$ver);

sub gen_version() {
	print "Generating release for version $ver\n";

	open IN, "ChangeLog" or return "error opening ChangeLog";
	open OUT, ">$body_path" or return "error creating $body_path";
	my $start=1;
	while (<IN>) {
		if ($start) {
			print OUT $_;
			$start = 0;
			next;
		}
		last if (m/^\S/);
		print OUT $_ or return "error writing to $body_path";
	}
	close OUT or return "error closing $body_path";

	return "";
}

my $ret = gen_version();

die($ret) if ($ret ne "");
