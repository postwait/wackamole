package example;

use strict;
use Data::Dumper;

sub dump_it {
    my $i = 1;
    while($_ = shift) {
        print "ARG$i: ".Dumper($_);
        $i++;
    }
}

1;
