//
//  UNRLE.CPP
//
//  Mark Nelson
//  March 8, 1996
//  http://web2.airmail.net/markn
//
// DESCRIPTION
// -----------
//
//  This program reverses the Run Length Encoding function on an
//  input file/stream, and sends the result to an output file
//  or stream.  In the input stream, any two consecutive
//  characters with the same value flag a run.  A byte following
//  those two characters gives the count of *additional*
//  repeat characters, which can be anything from 0 to 255.
//  This isn't an optimal RLE scheme, but it you have to like
//  its simplicity.
//
//  Using the RLE program as a front end to BWT avoids
//  pathologically slow sorts that occur when the input stream
//  has long sequences of identical characters.  The RLE has
//  also been used as a heuristic postprocessor from the MTF
//  program, before sending the stream to the entropy encoder.
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
//  file system into binary mode.  g++ already does this, your
//  UNIX C++ compiler might do this also.
//
//  Borland C++ 4.5 16 bit    : bcc -w unrle.cpp
//  Borland C++ 4.5 32 bit    : bcc32 -w unrle.cpp
//  Microsoft Visual C++ 1.52 : cl /W4 unrle.cpp
//  Microsoft Visual C++ 2.1  : cl /W4 unrle.cpp
//  g++                       : g++ -o unrle.cpp unrle.cpp
//
// Typical Use
// -----------
//
//  unari < compressed-file | unrle | unmtf | unbwt | unrle > raw-file
//
//

#include <stdio.h>
#include <fcntl.h>
#if !defined( unix )
#include <io.h>
#endif

int main( int argc, char *argv[] )
{
    fprintf( stderr, "Run length decoding " );
    if ( argc > 1 ) {
        freopen( argv[ 1 ], "rb", stdin );
        fprintf( stderr, "%s", argv[ 1 ] );
    } else
        fprintf( stderr, "stdin" );
    fprintf( stderr, " to " );
    if ( argc > 2 ) {
        freopen( argv[ 2 ], "wb", stdout );
        fprintf( stderr, "%s", argv[ 2 ] );
    } else
        fprintf( stderr, "stdout" );
    fprintf( stderr, "\n" );
#if !defined( unix )
    setmode( fileno( stdin ), O_BINARY );
    setmode( fileno( stdout ), O_BINARY );
#endif

    int last = 0;
    int c;
    int count;
    while ( ( c = getc( stdin ) ) >= 0 )  {
        putc( (char) c, stdout );
        if ( c == last ) {
            count = getc( stdin );
            while ( count-- > 0 )
                putc( (char) c, stdout );
        }
        last = c;
    }
    return 1;
}

