//
//  UNMTF.CPP
//
//  Mark Nelson
//  March 8, 1996
//  http://web2.airmail.net/markn
//
// DESCRIPTION
// -----------
//
//  This program performs a Move To Front decoding function on an
//  input file/stream, and sends the result to an output file
//  or stream.  The MTF decoder keeps an array of 256 characters
//  int the order that they have appeared.  Each time the encoder
//  sends a number, the decoder uses it to look up a character in
//  the corresponding position of the array, and outputs it.  That
//  character is then moved up to position 0 in the array, and all
//  the in-between characters are moved down a spot.
//
//  Both the encoder and decoder have to start with the order array
//  initialized to the same values.
//
//  This program takes two arguments: an input file and an output
//  file.  You can leave off one argument and send your output to
//  stdout.  Leave off two arguments and read your input from stdin
//  as well.
//
//  This program accompanies my article "Data Compression with the
//  Burrows-Wheeler Transform."
//
// Build Instructions
// ------------------
//
//  Define the constant unix for UNIX or UNIX-like systems.  The
//  use of this constant turns off the code used to force the MS-DOS
//  file system into binary mode.  g++ already does this, your UNIX
//  C++ compiler might also.
//
//  Borland C++ 4.5 16 bit    : bcc -w mtf.cpp
//  Borland C++ 4.5 32 bit    : bcc32 -w mtf.cpp
//  Microsoft Visual C++ 1.52 : cl /W4 mtf.cpp
//  Microsoft Visual C++ 2.1  : cl /W4 mtf.cpp
//  g++                       : g++ -o unmtf unmtf.cpp
//
// Typical Use
// -----------
//
//  unari < compressed-file | unrle | unmtf | unbwt | unrle > raw-file
//
//
#include <stdio.h>
#if !defined( unix )
#include <io.h>
#endif
#include <fcntl.h>

int main( int argc, char *argv[] )
{
    int c;

    fprintf( stderr, "Performing MTF decoding on " );
    if ( argc > 1 ) {
        freopen( argv[ 1 ], "rb", stdin );
        fprintf( stderr, "%s",  argv[ 1 ] );
    } else
        fprintf( stderr, "stdin" );
    fprintf( stderr, " to " );
    if ( argc > 2 ) {
        freopen( argv[ 2 ], "wb", stdout );
        fprintf( stderr, "%s",  argv[ 2 ] );
    } else
        fprintf( stderr, "stdout" );
    fprintf( stderr, "\n" );
#if !defined( unix )
    setmode( fileno( stdin ), O_BINARY );
    setmode( fileno( stdout ), O_BINARY );
#endif

    unsigned char order[ 256 ];
    int i;
    for ( i = 0 ; i < 256 ; i++ )
        order[ i ] = (unsigned char) i;
    while ( ( i = getc( stdin ) ) >= 0 )  {
//
// Find the char
//
        putc( order[ i ], stdout );
        c = order[ i ];
//
// Now shuffle the order array
//
        int j;
        for ( j = i ; j > 0 ; j-- )
            order[ j ] = order[ j - 1 ];
        order[ 0 ] = (unsigned char) c;
    }
    return 1;
}

