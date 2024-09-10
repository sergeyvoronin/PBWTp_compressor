#!/usr/bin/perl

my $infile = $ARGV[0];
my $key = $ARGV[1];

print "infile: $infile\n";
print "key: $key\n";

#$cmd = "rm -rf temp/inverse/ ; mkdir temp/inverse/";
#system($cmd);

print "running inv AC..\n";
$cmd = "./ac1 d $infile temp/inverse/inv_ac1_p$key";
print($cmd);
system($cmd);

print "running invRLE..\n";
$cmd = "./unrle0 < temp/inverse/inv_ac1_p$key > temp/inverse/inv_ac2_p$key";
print($cmd);
system($cmd);

print "running invMTF..\n";
$cmd = "./unmtf0 < temp/inverse/inv_ac2_p$key > temp/inverse/inv_ac3_p$key";
#$cmd = "./mtfzle1 -i temp/inverse/inv_ac2_p$key temp/inverse/inv_ac3_p$key";
print($cmd);
system($cmd);

print "running invRLE..\n";
#$cmd = "./unrle0 < temp/inverse/inv_ac3_p$key > temp/file_parts/part_$key.dat";
$cmd = "./unrle0 < temp/inverse/inv_ac3_p$key > temp/file_parts/megablock_$key.dat";
print($cmd);
system($cmd);

