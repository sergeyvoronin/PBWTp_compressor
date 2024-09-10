#!/usr/bin/perl

# Parallel decompression driver: takes folder of compressed megablocks, decompresses, reconstructs BWT output, and runs inverse BWT to reconstruct the original input. 
# Sergey Voronin, 2024 
use Cwd;

if (scalar(@ARGV) < 3){
    print "need 4 arguments: infolder outfile block_size nthreads\n";
    exit(1);
}

my $infolder = $ARGV[0];
my $outfile = $ARGV[1];
my $blsize = $ARGV[2]; # e.g. "2.0MB";
my $nthreads = $ARGV[3];
my $megasplit = "cluster"; # set "cluster" or "parts"
my $psout;
my @keys = ();
open (FILE, "> temp/keys.txt");
close(FILE);

print "infolder: $infolder\n";
print "outfile: $outfile\n";
print "nthreads: $nthreads\n";

$cmd = "rm -f temp/file_parts/*dat ; rm -f temp/inverse/*";
system($cmd);

my @running_processes;
foreach my $file (glob("$infolder/*bzp")) {
    while (scalar(@running_processes) >= $nthreads) {
        for my $pid (waitpid(-1, 0)) {  # Wait for *any* child process to finish
            splice(@running_processes, 0, grep { $_ == $pid } @running_processes);
        }
    }

		# Extract the part number from the filename
		print("processing $file..\n");
		$key = $file; $key =~ s/\D+//g;
    push @keys, $key;
    printf("file: %s, key: %s\n", $file, $key);

    my ($part_num) = $file =~ /part(\d+)\.dat$/;
    my $command = "./decompress_one.pl $file $key"; 
		print("$command\n");

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

# write the keys
open (FILE, "> temp/keys.txt");
foreach $key (@keys){
  print FILE "$key\n";
}
close(FILE);

printf("outfile: %s\n", $outfile);
#print("exiting\n");
#exit();

$psout = `ps aux | grep decompress_one | grep -v grep`; 
sleep(2);
while(length($output)>1){
    sleep(2);
    print "waiting for decompress_one finish prior to inverse bwt\n";
    $psout = `ps aux | grep decompress_one | grep -v grep`;
}

printf("outfile: %s\n", $outfile);
my $cwd = getcwd();
$cmd = "./unbwta $cwd/temp/file_parts/* $outfile $blsize";
printf("ibwt cmd (being performed below..): $cmd\n");

my $part_dir = "$cwd/temp/file_parts/";

# run reconstruct script
if($megasplit =~ m/cluster/){
	print("run reconstruct and ibwt..\n");
	$cmd = "cp $infolder/metadata.json temp/file_parts/";
	system($cmd);
	$cmd = "./reconstruct_from_mblocks1.py temp/file_parts/ temp/bwt_recon.out";
	print("$cmd\n");
	system($cmd);
	$cmd = " ./unbwtb temp/bwt_recon.out $outfile $blsize";
	print("$cmd\n");
	system($cmd);
} else {
	$cmd = "./unbwtb $cwd/temp/file_parts/* $outfile $blsize";
	printf("ibwt cmd (being performed below..): $cmd\n");

	my $part_dir = "$cwd/temp/file_parts/";
	@running_processes = ();
	foreach my $file (glob("$part_dir/part*.dat")) {
			while (scalar(@running_processes) >= $nthreads) {
					for my $pid (waitpid(-1, 0)) {  # Wait for *any* child process to finish
							splice(@running_processes, 0, grep { $_ == $pid } @running_processes);
					}
			}

			# Extract the part number from the filename
			my ($part_num) = $file =~ /part(\d+)\.dat$/;
			my $file_out = "$part_dir/uncomp$part_num.dat";
			#my $command = "./unbwta $file $file_out $blsize"; 
			my $command = "./unbwtb $file $file_out $blsize"; 
			print("$command\n");
			

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

	# cat uncomp parts into output file
	print("writing output to $outfile.\n");
	sleep(1);
	my $command = "cat $part_dir/uncomp*dat > $outfile";
	system($command);
}

