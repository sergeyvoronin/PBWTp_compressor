#!/bin/bash

gcc BWTap2b.c -o exbwtap2 -pthread -lm
gcc unbwtpa.c -o unbwta -lm
gcc unbwtpb.c -o unbwtb -lm
gcc mtf1.c -o mtf1 -Os
gcc mtf2.c -o mtf2 -Os
gcc mtf_and_zle1.c -o mtfzle1 -fopenmp -Os
gcc arith_adapt1.c -o ac1 -Os

g++ nelson/RLE.CPP -o rle0 
g++ nelson/UNRLE.CPP -o unrle0 
g++ nelson/UNMTF.CPP -o unmtf0 
