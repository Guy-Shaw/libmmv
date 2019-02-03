# Filename: src/cmd/libtest/mmvtest.pm
#
# Project: libmmv
# Brief: A collection of subroutines and data common to testing mmv commands
#
# Copyright (C) 2016 Guy Shaw
# Written by Guy Shaw <gshaw@acm.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

package mmvtest;

require 5.0;
use strict;
use warnings;
use Exporter;
use vars qw(@ISA @EXPORT @EXPORT_OK $VERSION);

use File::Spec::Functions qw(splitpath catfile);

$VERSION = '0.01_01';

@ISA = qw/ Exporter /;
@EXPORT = qw(
    set_print_fh
    eprint
    eprintf
    dprint
    dprintf
    sname
    yyyymmddhhmmss
    fresh_tmpdir
    show_test_results
    write_new_file
    show_file
    show_mmv_stdout_and_stderr
    grep_file
    grep_files
);


our $eprint_fh;
our $dprint_fh;

my $program;
my $exe;
my $test_path;
my $test_name;
my $subtest;

our $leader = '  ' . ('. ' x 30);

#:subroutines:#

# Decide how to direct eprint*() and dprint*() functions.
# If STDOUT and STDERR are directed to the same "channel",
# then eprint*() and dprint*() should be tied to the same file handle.
#
# Otherwise, buffering could cause a mix of STDOUT and STDERR to
# be written out of order.
#
sub set_print_fh {
    my @stdout_statv;
    my @stderr_statv;
    my $stdout_chan;
    my $stderr_chan;

    @stdout_statv = stat(*STDOUT);
    @stderr_statv = stat(*STDERR);
    $stdout_chan = join(':', @stdout_statv[0, 1, 6]);
    $stderr_chan = join(':', @stderr_statv[0, 1, 6]);
    if (!defined($eprint_fh)) {
        $eprint_fh = ($stderr_chan eq $stdout_chan) ? *STDOUT : *STDERR;
    }
    if (!defined($dprint_fh)) {
        $dprint_fh = ($stderr_chan eq $stdout_chan) ? *STDOUT : *STDERR;
    }
}

sub eprint {
    if (-t $eprint_fh) {
        print {$eprint_fh} "\e[01;31m\e[K", @_, "\e[m\e[K";
    }
    else {
        print {$eprint_fh} @_;
    }
}

sub eprintf {
    if (-t $eprint_fh) {
        print  {$eprint_fh}  "\e[01;31m\e[K";
        printf {$eprint_fh} @_;
        print  {$eprint_fh}  "\e[m\e[K";
    }
    else {
        printf {$eprint_fh} @_;
    }
}

sub dprint {
    print {$dprint_fh} @_ if ($main::debug);
}

sub dprintf {
    printf {$dprint_fh} @_ if ($main::debug);
}

sub sname {
    my ($path) = @_;
    my ($vol, $dir, $sfn) = splitpath($path);
    return $sfn;
}

sub yyyymmddhhmmss {
    my ($time) = @_;
    my ($sec, $min, $hour, $mday, $mon, $year) = localtime($time);

    my $fmt = '%4u%02u%02u%02u%02u%02u'; # Equivalent to date '+%Y%m%d%H%M%S'
    sprintf($fmt, $year + 1900, $mon + 1, $mday, $hour, $min, $sec);
}

sub fresh_tmpdir {
    if (-e 'tmp') {
        my $tmp_backup;

        $tmp_backup = 'tmp-' . yyyymmddhhmmss(time);
        rename('tmp', $tmp_backup);
    }

    mkdir('tmp', 0777);
}

sub show_test_results {
    my ($test, $subtest, $err) = @_;
    my $show;
    if (defined($subtest) && $subtest ne '') {
        $show = sprintf("Test %s (%s)", $test, $subtest);
    }
    else {
        $show = $test;
    }

    if ((length($show) % 2) == 1) {
        $show .= ' ';
    }
    $show .= $leader;
    print substr($show, 0, 60), ' ', ($err ? 'failed' : 'OK'), ".\n";
}

sub write_new_file {
    my ($fname) = shift;
    my $fh;

    if (!open($fh, '>', $fname)) {
        eprint "open(> '${fname}') failed; $!\n";
        exit 2;
    }

    print {$fh} @_;
    close $fh;
}

sub show_file {
    my ($fname) = @_;
    my $fh;

    if (!open($fh, '<', $fname)) {
        eprint "open(< '${fname}') failed; $!\n";
        exit 2;
    }

    while (<$fh>) {
        print $_;
    }
    close $fh;
}

sub show_mmv_stdout_and_stderr {
    system('ls', '-dlh', 'mmv.out', 'mmv.err');
    if (-s 'mmv.out') {
        show_file('mmv.out');
    }
    if (-s 'mmv.err') {
        show_file('mmv.err');
    }
}

sub grep_file {
    my ($pattern_str, $fname) = @_;
    my $fh;
    my $re;
    my $found;

    $re = qr/$pattern_str/;
    if (!open($fh, '<', $fname)) {
        eprint "open(< '${fname}') failed; $!\n";
        exit 2;
    }

    $found = 0;
    while (<$fh>) {
        if (m{$re}) {
            $found = 1;
            last;
        }
    }
    close $fh;
    return $found;
}

sub grep_files {
    my $pattern_str = shift;
    for my $fname (@_) {
        if (grep_file($pattern_str, $fname)) {
            return 1;
        }
    }
    return  0;
}

1;
