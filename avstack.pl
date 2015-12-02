#!/usr/bin/perl -w
# avstack.pl: AVR stack checker
# Copyright (C) 2013 Daniel Beer <dlbeer@gmail.com>
#
# Permission to use, copy, modify, and/or distribute this software for
# any purpose with or without fee is hereby granted, provided that the
# above copyright notice and this permission notice appear in all
# copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
# PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#
# Usage
# -----
#
# This script requires that you compile your code with -fstack-usage.
# This results in GCC generating a .su file for each .o file. Once you
# have these, do:
#
#    ./avstack.pl <object files>
#
# This will disassemble .o files to construct a call graph, and read
# frame size information from .su. The call graph is traced to find, for
# each function:
#
#    - Call height: the maximum call height of any callee, plus 1
#      (defined to be 1 for any function which has no callees).
#
#    - Inherited frame: the maximum *inherited* frame of any callee, plus
#      the GCC-calculated frame size of the function in question.
#
# Using these two pieces of information, we calculate a cost (estimated
# peak stack usage) for calling the function. Functions are then listed
# on stdout in decreasing order of cost.
#
# Functions which are recursive are marked with an 'R' to the left of
# them. Their cost is calculated for a single level of recursion.
#
# The peak stack usage of your entire program can usually be estimated
# as the stack cost of "main", plus the maximum stack cost of any
# interrupt handler which might execute.

use strict;

# Configuration: set these as appropriate for your architecture/project.

my $objdump = "tools/gcc-arm-none-eabi-4_9-2015q1/bin/arm-none-eabi-objdump";
my $call_cost = 0;

# First, we need to read all object and corresponding .su files. We're
# gathering a mapping of functions to callees and functions to frame
# sizes. We're just parsing at this stage -- callee name resolution
# comes later.

my %frame_size;     # "func@file" -> size
my %call_graph;     # "func@file" -> {callees}
my %addresses;      # "addr@file" -> "func@file"
my %makes_fp_calls; # "func@file" -> 1

my %global_name;    # "func" -> "func@file"
my %ambiguous;      # "func" -> 1

foreach (@ARGV) {
    # Disassemble this object file to obtain a callees. Sources in the
    # call graph are named "func@file". Targets in the call graph are
    # named either "offset@file" or "funcname". We also keep a list of
    # the addresses and names of each function we encounter.
    my $objfile = $_;
    my $source;

    open(DISASSEMBLY, "$objdump -dr $objfile|") ||
	die "Can't disassemble $objfile";
    while (<DISASSEMBLY>) {
	chomp;

	if (/^([0-9a-fA-F]+) <(.*)>:/) {
	    my $a = $1;
	    my $name = $2;

	    $source = "$name\@$objfile";
	    $call_graph{$source} = {};
	    $ambiguous{$name} = 1 if defined($global_name{$name});
	    $global_name{$name} = "$name\@$objfile";

	    $a =~ s/^0*//;
	    $addresses{"$a\@$objfile"} = "$name\@$objfile";
	}

	if (/: R_[A-Za-z0-9_]+_CALL[ \t]+(.*)/) {
	    my $t = $1;

	    if ($t eq ".text") {
		$t = "\@$objfile";
	    } elsif ($t =~ /^\.text\+0x(.*)$/) {
		$t = "$1\@$objfile";
	    }

	    $call_graph{$source}->{$t} = 1;
	}

	if (/blx[ \t]+/) {
	    $makes_fp_calls{$source} = 1;
	    print "taint $source\n";
	}
    }
    close(DISASSEMBLY);

    # Extract frame sizes from the corresponding .su file.
    if ($objfile =~ /^(.*).o$/) {
	my $sufile = "$1.su";

	open(SUFILE, "<$sufile") || die "Can't open $sufile";
	while (<SUFILE>) {
	    $frame_size{"$1\@$objfile"} = $2 + $call_cost
		if /^.*:([^\t ]+)[ \t]+([0-9]+)/;
	}
	close(SUFILE);
    }
}

# In this step, we enumerate each list of callees in the call graph and
# try to resolve the symbols. We omit ones we can't resolve, but keep a
# set of them anyway.

my %unresolved;

foreach (keys %call_graph) {
    my $from = $_;
    my $callees = $call_graph{$from};
    my %called;

    foreach (keys %$callees) {
	my $t = $_;

	if (defined($addresses{$t})) {
	    $called{$addresses{$t}} = 1;
	} elsif (defined($global_name{$t})) {
	    $called{$global_name{$t}} = 1;
	    warn "Ambiguous resolution: $t" if defined ($ambiguous{$t});
	} elsif (defined($call_graph{$t})) {
	    $called{$t} = 1;
	} else {
	    $called{$t} = 1;
	    $unresolved{$t} = 1;
	}
    }

    $call_graph{$from} = \%called;
}

# Create fake edges and nodes to account for dynamic behaviour.
$call_graph{"INTERRUPT"} = {};

foreach (keys %call_graph) {
    $call_graph{"INTERRUPT"}->{$_} = 1 if /^__vector_/;
}

# Trace the call graph and calculate, for each function:
#
#    - inherited frames: maximum inherited frame of callees, plus own
#      frame size.
#    - height: maximum height of callees, plus one.
#    - recursion: is the function called recursively (including indirect
#      recursion)?

my %has_caller;
my %visited;
my %total_cost;
my %call_depth;

sub trace {
    my $f = shift;

    if ($visited{$f}) {
	$visited{$f} = "R" if $visited{$f} eq ".";
	return;
    }

    $visited{$f} = ".";

    my $max_depth = 0;
    my $max_frame = 0;

    my $tainted_by_fpcall = 0;
    my $tainted_by_unknown = 0;

    $tainted_by_fpcall = 1 if $makes_fp_calls{$f};

    my $targets = $call_graph{$f} || die "Unknown function: $f";
    if (defined($targets)) {
	foreach (keys %$targets) {
	    my $t = $_;

	    $has_caller{$t} = 1;
	    trace($t);

	    $tainted_by_unknown = 1 if ($visited{$t} eq '!') or ($visited{$t} eq '$');
	    $tainted_by_fpcall = 1 if $visited{$t} eq '(';

	    if ($visited{$t} eq '(') {
		    print "tainting $f because $t\n";
	    }

	    my $is = $total_cost{$t};
	    my $d = $call_depth{$t};

	    $max_frame = $is if $is > $max_frame;
	    $max_depth = $d if $d > $max_depth;
	}
    }

    $call_depth{$f} = $max_depth + 1;
    $total_cost{$f} = $max_frame + ($frame_size{$f} || 0);
    
    if ($visited{$f} eq ".") {
        $visited{$f} = " ";

	$visited{$f} = '$' if $tainted_by_unknown;
	$visited{$f} = '(' if $tainted_by_fpcall;
    }
}

# This is a hack.  Assume each of the unresolved functions use 32 bytes of
# stack space.  This is somewhat valid when it's going to a library math call
# which can be assumed to use a few registers.
foreach (keys %unresolved) {
    $total_cost{$_} = 32;
    $call_depth{$_} = 1;

    $visited{$_} = '!';
    $visited{$_} = ' ' if (/memset/);

}

foreach (keys %call_graph) { trace $_; }

# Now, print results in a nice table.
printf "  %-30s %8s %8s %8s\n",
    "Func", "Cost", "Frame", "Height";
print "------------------------------------";
print "------------------------------------\n";

my $max_iv = 0;
my $main = 0;

foreach (sort { $total_cost{$b} <=> $total_cost{$a} } keys %visited) {
    my $name = $_;

    if (/^(.*)@(.*)$/) {
	$name = $1 unless $ambiguous{$name};
    }

    my $tag = $visited{$_};
    my $cost = $total_cost{$_};

    $name = $_ if $ambiguous{$name};
    $tag = ">$tag" unless $has_caller{$_};
    $tag = "\@$tag" if $makes_fp_calls{$_};

    if (/^__vector_/) {
	$max_iv = $cost if $cost > $max_iv;
    } elsif (/^main@/) {
	$main = $cost;
    }

    $name=$_ if ($ambiguous{$name});

    # eliminate some known bogus entry functions
    if ($tag eq "> ") {
	next if ($cost == 0);
	next if (/SetDefaults/);
    }

    printf "%s %-30s %8d %8d %8d\n", $tag, $name, $cost,
	$frame_size{$_} || 0, $call_depth{$_};
}

print "\n";

print "Peak execution estimate (main + worst-case IV):\n";
printf "  main = %d, worst IV = %d, total = %d\n",
      $total_cost{$global_name{"main"}},
      $total_cost{"INTERRUPT"},
      $total_cost{$global_name{"main"}} + $total_cost{"INTERRUPT"};

print "\n";

print "The following functions were not resolved:\n";
foreach (keys %unresolved) { print "  $_\n"; }
