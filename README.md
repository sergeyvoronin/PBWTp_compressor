# PBWTp_compressor
Experimental Burrows-Wheeler based parallel lossless compressor. Sergey Voronin / 2024. 
License: https://www.gnu.org/licenses/gpl-3.0.en.html

Builds on Mark Nelson's classical implementation.
Enhancements such as parallel BWT, byte frequency grouping, move to front and zero length encoding, and arithmetic coding. 
Compresses a file (or tar'd folder) to an ouput folder consisting of several compressed blocks. Decompresses the folder contents back to input file. 

To use, first, make sure the following subdirectories exist:
mkdir temp/; mkdir temp/file_parts; mkdir temp/inverse; mkdir out_cmp/
temp/ holds temp data and can be cleared with ./cleanup.sh after each run. 
Megablock construction uses Python's sklearn.cluster and concurrent.futures which can be installed with pip. 
For large inputs, it may be better to use splitf_in_mblocks0.pl, which uses basic Perl to efficiently bundle BWT blocks for use with the inverse BWT. 
This is done by setting the variable $megasplit to "parts" in the compress and decompress drivers. 

Compress / decompress sequence:
# need 6 arguments: infile outfolder blsize_for_bwt nparts_per_mblock max_mblock_size nthreads
$ ./parallel_compress.pl comp_data/comb2.dat out_cmp/ 2.0MB 8 20MB 8

# review compressed data:
$ ls out_cmp/
comp_0.bzp  comp_1.bzp  comp_2.bzp  comp_3.bzp  comp_4.bzp  comp_5.bzp  comp_6.bzp  comp_7.bzp  metadata.json
$ du -hs comp_data/comb2.dat out_cmp/
111M 
13M 

# now decompress output folder to recover the original. 
# need 4 arguments: infolder outfile block_size nthreads
$ ./parallel_decompress.pl out_cmp/ out.rec 2.0MB 4

# Size compare and diff reconstruction:
$ diff comp_data/comb2.dat out.rec

