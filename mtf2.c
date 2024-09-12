/*
 * optimized move to front implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// # max len bytes 
#define MAX 40980
// # symbols in alphabet 
#define NO_OF_CHARS 256
// EOF symbol 
#define EOF_SYMBOL    (NO_OF_CHARS + 1)
// Total number of symbols
#define NO_OF_SYMBOLS (NO_OF_CHARS + 1)

// The tables for recoding
unsigned char index_to_char[NO_OF_SYMBOLS];
int char_to_index[NO_OF_CHARS];

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
/* must call the flushOutput(FILE *fd) to flush out the buffer  */                                                    
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

    // Initialize dict (location to character) and map (character to location)
    for(i = 0; i < 256; i++){
        dict[i] = i; // location to character
        map[i] = i;  // character to location            
    }
        
    out = fopen(outfile, "w+b");
    fd = fopen(infile, "rb");

    if (fd == NULL || out == NULL)
        return;

    int index;
    int charVal;
    unsigned char buffer;

    for (;;)
    {    
        // Get the next character from input
        rc = getChar(fd, &buffer); 
        if (rc == -1)
            break;

        charVal = (int)buffer;
        index = map[charVal];  // Look up index of the character from map

        // Write the index (MTF encoded value) to the output
        putChar(out, index);

        // Move the accessed character to the front of dict
        for(check = index; check != 0; check--) {
            // Move each character up in dict
            map[dict[check - 1]]++;  // Increase the index of the character being shifted
            dict[check] = dict[check - 1];  // Shift dict entries
        }
        dict[0] = charVal;       // Place accessed character at front
        map[charVal] = 0;        // Update map with new location of charVal
    }

    // Flush the output buffer and close files
    flushOutput(out);
    fclose(fd);
}


/* inverse function */
void imtf2(const char *infile, const char *outfile) {
    FILE *in = fopen(infile, "rb");
    FILE *out = fopen(outfile, "wb");
    if (!in || !out) {
        perror("File error");
        exit(1);
    }

    // Initialize dictionary
    unsigned char dict[256];
    for (int i = 0; i < 256; i++) {
        dict[i] = i;
    }

    unsigned char ch;
    while (fread(&ch, 1, 1, in) == 1) {
        int index = (int)ch;

        // Check for out-of-bounds access
        if (index < 0 || index > 255) {
            fprintf(stderr, "Error: Invalid index %d in input file.\n", index);
            fclose(in);
            fclose(out);
            exit(1);
        }

        // Retrieve the corresponding character from the dictionary
        unsigned char charVal = dict[index];

        // Write the character to the output file
        putChar(out, charVal);

        // Move the accessed character to the front of the dictionary
        for (int j = index; j > 0; j--) {
            dict[j] = dict[j - 1];
        }
        dict[0] = charVal;
    }

    // Flush any remaining output
    flushOutput(out);

    fclose(in);
    fclose(out);
}


//------------------------------------------------------------
// main command line inputs
int main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("Usage: %s -f|-i infile outfile\n", argv[0]);
        return 1;
    }

    char *mode = argv[1];
    char *infile = argv[2];
    char *outfile = argv[3];

    if (strcmp(mode, "-f") == 0) {
        mtf2(infile, outfile);
    } else if (strcmp(mode, "-i") == 0) {
        imtf2(infile, outfile);
    } else {
        fprintf(stderr, "Invalid mode. Use -f for forward or -i for inverse.\n");
        return 1;
    }

   return 0;
}

