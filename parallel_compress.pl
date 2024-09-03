#!/usr/bin/perl

# Parallel compression driver: clusters BWT output in megablocks and compresses them in parallel
# Sergey Voronin, 2024 

if (scalar(@ARGV) < 6){
    print "need 6 arguments: infile outfolder blsize_for_bwt nparts_per_mblock max_mblock_size nthreads\n";
    exit(1);
}

my $infile = $ARGV[0];
my $outfolder = $ARGV[1];
my $blsize = $ARGV[2];
my $nparts_per_mblock = $ARGV[3];
my $max_mblock_size = $ARGV[4];
my $nthreads = $ARGV[5];
my $megasplit = "cluster"; # set "cluster" or "parts"

print "infile: $infile\n";
print "outfolder: $outfolder\n";
print "blsize for BWT (KB/MB): $blsize\n";
print "num parts per megablock: $nparts_per_mblock\n";
print "max size of megablock (KB/MB): $max_mblock_size\n";
print "nthreads: $nthreads\n";

$cmd = "mkdir $outfolder";
system($cmd);

my ( $blsize_num ) = ( $blsize =~ /(\d+(?:\.\d+)?)/ );
print("blsize_num = $blsize_num\n");

# clean up
$cmd = "./cleanup.sh";
system($cmd);
$cmd = "rm -f $outfolder/*";
system($cmd);

# write bwt settings
my $bwt_settings = "temp/bwt_settings.txt";
open(FH, '>', $bwt_settings) or die $!;
print FH "$infile\n";
print FH "$outfolder\n";
print FH "$blsize\n";
print FH "$nparts_per_mblock\n";
print FH "$nthreads\n";
close(FH);

# run multi-threaded bwt
my $bwt_out = "temp/bwt_out.dat";
$cmd = "./exbwtap2 $infile $bwt_out $blsize";
print("$cmd\n");
system($cmd);
print("finished BWT..\n");
sleep(.5);

# split BWT output in parts
$n = $nparts_per_mblock; 
print("splitting $bwt_out in $n parts with max mblock size: $max_mblock_size.\n");
if($megasplit =~ m/cluster/){
$cmd = "./splitf_in_mblocks1.py $bwt_out temp/file_parts/ $blsize $n $max_mblock_size";
} else {
$cmd = "./splitf_in_mblocks0.pl $bwt_out temp/bwt_log.txt $n temp/file_parts/";
}
print("$cmd\n");
system($cmd);
sleep(.25);
$cmd = "cp temp/file_parts/metadata.json $outfolder/";
system($cmd);

# compress the megablocks in parallel
my @running_processes;
foreach my $file (glob("temp/file_parts/*dat")) {
    while (scalar(@running_processes) >= $nthreads) {
        for my $pid (waitpid(-1, 0)) {  # Wait for *any* child process to finish
            splice(@running_processes, 0, grep { $_ == $pid } @running_processes);
        }
    }

		# Extract the part number from the filename
		$key = $file; $key =~ s/\D+//g;
    push @keys, $key;
    printf("file: %s, key: %s\n", $file, $key);

    my ($part_num) = $file =~ /part(\d+)\.dat$/;
    my $command = "./compress_one.pl $file $key $outfolder"; 

    my $pid = fork();

    if (!defined $pid) {
        die "Fork failed: $!\n";
    } elsif ($pid == 0) {
        # Child process
        system($command);
        exit;
    } else {
        # Parent process
        push @running_processes, $pid; 
    }
}

# Wait for all remaining child processes to finish
foreach my $pid (@running_processes) {
    waitpid($pid, 0);
}

