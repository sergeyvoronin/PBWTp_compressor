/*
 * optimized move to front implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// The number of bits in "register"
#define BITS_IN_REGISTER 16

// The max value in register
#define TOP_VALUE (((long) 1 << BITS_IN_REGISTER) - 1)

// set ranges
#define FIRST_QTR (TOP_VALUE / 4 + 1)
#define HALF (2 * FIRST_QTR)
#define THIRD_QTR (3 * FIRST_QTR)

// # symbols in alphabet 
#define NO_OF_CHARS 256
// EOF symbol 
#define EOF_SYMBOL    (NO_OF_CHARS + 1)
// Total number of symbols
#define NO_OF_SYMBOLS (NO_OF_CHARS + 1)

// The limit of frequency for scale
#define MAX_FREQUENCY 16383

// The tables for recoding
unsigned char index_to_char [NO_OF_SYMBOLS];
int char_to_index [NO_OF_CHARS];

// tables of freq 
int cum_freq [NO_OF_SYMBOLS + 1];
int freq [NO_OF_SYMBOLS + 1];

// limit registers  
long low, high;
long value;

// registers for bit i/o ops 
long bits_to_follow;
unsigned char buffer;
int bits_to_go;
int garbage_bits;

// file ptrs 
FILE *in, *out;
FILE *fd;

#define MAX 40980
 unsigned char inputBuffer [MAX];
 unsigned char *currentPtr = 0;
 int dataLeft = 0;
 int rc ;
 unsigned char outputBuffer [MAX];
 
 int outputLength = 0;

/* ******************************************************* */
/* GetChar - retrieve a char a time with a large buffer    */
/*                                                         */
/* ******************************************************* */
char getChar(FILE *fd, unsigned char *ch)
{

      if (dataLeft == 0 ) {
	      rc = fread(inputBuffer, 1, MAX, fd);
	
		  if (rc >0 ) {
		      currentPtr = inputBuffer;
			  dataLeft = rc;
		  } else {
		    // no more data
		    return -1;
		  }
	  }
	  *ch = (char)*currentPtr++;
	  dataLeft--;
 
	return 1;
 }
 
/* ************************************************************ */
/* flushOutput - Flush out the output buffer                    */                                                   
/* ************************************************************ */
 void flushOutput(FILE *fd){
   if (outputLength > 0) 
      fwrite(outputBuffer, 1, outputLength, fd);
   return;
 }
 
/* ************************************************************ */
/* putChar - output a char a time with a large buffer           */
/* must call the fluchOutput(FILE *fd) to flush out the buffer  */                                                    
/* ************************************************************ */
int putChar(FILE *out, unsigned char ch)
{
 
      if (outputLength >= MAX ) {
	      rc = fwrite(outputBuffer, 1, MAX, out);
		   //printf("outputLength = %d, rc=%d\n", outputLength, rc);
		  if (rc >=0 ) {
		      outputLength = 0;
		
		  } else {
		    // no more data
		    return -1;
		  }
	  }
	  outputBuffer[outputLength] = ch;
	  outputLength++;
  
	return 1;
 }


void mtf2(char *infile, char *outfile) 
{
        int i, check;
		int dict[256];
        int map[256];

		for(i = 0; i < 256; i++){
			dict[i] = i; // location to character
            map[i] = i; // character to location			
		}
			
	    out = fopen ( outfile, "w+b");
  
        fd = fopen(infile,  "rb");

        if (fd == NULL || out == NULL)
          return;
  		int index;
		int charVal;
	
        for (;;)
        {	

			rc = getChar(fd, &buffer); 
            if (rc == -1)
                 break;


			charVal = (int) buffer;
			index = map[charVal];
				
			for(check = index; check != 0; check--) {
				map[dict[check -1]] ++;
				dict[check] = dict[check-1];

			}
					
			dict[0] = charVal;
            map[charVal] = 0;			
			putChar(out, index);

        }
	    flushOutput(out);
        fclose(fd);
}


//------------------------------------------------------------
// main command line inputs
int main(int argc, char* argv[])
{
   if (argc == 3) {
	   printf ("\n mtf using: %s  %s %s\n", argv[0], argv[1], argv[2]);
       mtf2(argv[1],argv[2]);
   } else {
	   printf ("\n wrong number of arguments %d\n", argc);
	   printf ("\n Usage: %s infile outfile\n", argv[0]);
    exit (0);
   }
   return 0;

}

