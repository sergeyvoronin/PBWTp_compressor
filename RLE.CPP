//
//  RLE.CPP
//
//  Mark Nelson
//  March 8, 1996
//  http://web2.airmail.net/markn
//
// DESCRIPTION
// -----------
//
//  This program performs a Run Length Encoding function on an
//  input file/stream, and sends the result to an output file
//  or stream.  In the output stream, any two consecutive
//  characters with the same value flag a run.  A byte following
//  those two characters gives the count of *additional*
//  repeat characters, which can be anything from 0 to 255.
//
//  Using the RLE program as a front end to BWT avoids
//  pathologically slow sorts that occur when the input stream
//  has long sequences of identical characters. (Which means
//  comparison functions have to spend lots of time on a pair
//  of strings before deciding who is larger.)
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
//  file system into binary mode. g++ already does this, your UNIX C++
//  compiler might also.
//
//  Borland C++ 4.5 16 bit    : bcc -w rle.cpp
//  Borland C++ 4.5 32 bit    : bcc32 -w rle.cpp
//  Microsoft Visual C++ 1.52 : cl /W4 rle.cpp
//  Microsoft Visual C++ 2.1  : cl /W4 rle.cpp
//  g++                       : g++ -o rle rle.cpp
//
// Typical Use
// -----------
//
//  rle < raw-file | bwt | mtf | rle | ari > compressed-file
//
//

#include <stdio.h>
#if !defined( unix )
#include <io.h>
#include <fcntl.h>
#endif

int main( int argc, char *argv[] )
{
    fprintf( stderr, "Run length encoding " );
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
    while ( ( c = getc( stdin ) ) >= 0 )  {
        putc( (char) c, stdout );
        if ( c == last ) {
            int count = 0;
            while ( count < 255 &&
                    ( ( c = getc( stdin ) ) >= 0 ) ) {
                if ( c == last )
                    count++;
                else
                    break;
            }
            putc( (char) count, stdout );
            if ( count != 255 && c >= 0 )
                putc( (char) c, stdout );
        }
        last = c;
    }
    return 1;
}

