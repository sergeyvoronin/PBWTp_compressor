#!/usr/bin/perl
use strict;
use warnings;
use Fcntl ':seek';

# Split input in parts, with each part invertible by inverse BWT. 
# Takes BWT transformed output and splits into number of parts.
my ($input_file, $log_file, $num_parts, $out_dir) = @ARGV;
die "Usage: $0 <input_file> <log_file> <number_of_parts> <out_dir>\n" unless $input_file && $log_file && $num_parts && $out_dir;

my $output_prefix = "$out_dir/megablock";     # Prefix for output files
my $blocks_per_megablock = $num_parts;        # Number of small blocks per megablock

open my $log, '<', $log_file or die "Could not open log file: $!";
open my $input, '<', $input_file or die "Could not open input file: $!";
binmode $input; # Ensure binary mode for non-text data

my $mb = 0;                          # Megablock counter
my $block_count = 0;                 # Counter for blocks within a current megablock
my $output_file;                     # Current output file name
my $megablock_data = '';             # Buffer to accumulate megablock data

while (my $line = <$log>) {
    if ($line =~ /Block \d+: Start = (\d+), End = (\d+)/) {
        my $start_byte = $1;
        my $end_byte = $2;
        my $length = $end_byte - $start_byte;

        # Seek to the start position in the input file
        seek($input, $start_byte, SEEK_SET);

        # Read the specified amount of data
        my $buffer;
        read($input, $buffer, $length);

        # Accumulate data into megablock buffer
        $megablock_data .= $buffer;
        $block_count++;

        # Check if we have reached the required number of blocks for this megablock
        if ($block_count == $blocks_per_megablock) {
            # Write the megablock to a file
            $output_file = "${output_prefix}_${mb}.dat";
            open my $output, '>', $output_file or die "Could not open output file: $!";
            binmode $output;
            print $output $megablock_data;
            close $output;

            print("Extracted megablock $mb to $output_file\n");

            # Reset for the next megablock
            $megablock_data = '';
            $block_count = 0;
            $mb++;
        }
    }
}

# Handle any remaining data in the buffer for the last megablock
if ($megablock_data ne '') {
    $output_file = "${output_prefix}_${mb}.dat";
    open my $output, '>', $output_file or die "Could not open output file: $!";
    binmode $output;
    print $output $megablock_data;
    close $output;

    print "Extracted remaining megablock to $output_file\n";
}

close $input;
close $log;

