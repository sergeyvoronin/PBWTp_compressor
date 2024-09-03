#!/usr/bin/perl

my $infile = $ARGV[0];
my $key = $ARGV[1];
my $outfolder = $ARGV[2];

print "infile: $infile\n";
print "key: $key\n";
print "outfolder: $outfolder\n";

print "running RLE..\n";
$cmd = "./rle0 < $infile > $infile.prle";
system($cmd);

print "running MTF..\n";
#$cmd = "./mtf1 $infile.prle temp/bwt_res_$key.mtf";
$cmd = "./mtfzle1 -f $infile.prle temp/bwt_res_$key.mtf";
system($cmd);

print "running RLE..\n";
$cmd = "./rle0 < temp/bwt_res_$key.mtf > temp/bwt_res2_$key.mtf";
system($cmd);

print "running AC..\n";
$cmd = "./ac1 e temp/bwt_res2_$key.mtf $outfolder/comp_$key.bzp";
system($cmd);

