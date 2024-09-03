#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <limits.h>

#define MAX_RUN_LENGTH 255
/* Move to front and zero length coding implementation */

unsigned char ZLE_MARKER; // Dynamic ZLE marker

#define MAX 40980
unsigned char inputBuffer[MAX];
unsigned char *currentPtr = 0;
int dataLeft = 0;
int rc;
unsigned char outputBuffer[MAX];
int outputLength = 0;

unsigned char buffer;

/* Reads a character from the input file using a buffer for efficiency. */
char getChar(FILE *fd, unsigned char *ch) {
    if (dataLeft == 0) {
        rc = fread(inputBuffer, 1, MAX, fd);
        if (rc > 0) {
            currentPtr = inputBuffer;
            dataLeft = rc;
        } else {
            return -1;
        }
    }
    *ch = (char)*currentPtr++;
    dataLeft--;
    return 1;
}

void flushOutput(FILE *fd) {
    if (outputLength > 0) {
        fwrite(outputBuffer, 1, outputLength, fd);
        outputLength = 0;
    }
}

/* Writes a character to the output file using a buffer for efficiency. */
int putChar(FILE *out, unsigned char ch) {
    if (outputLength >= MAX) {
        flushOutput(out);
    }
    outputBuffer[outputLength++] = ch;
    return 1;
}

void encodeSymbol(FILE *out, int index) {
    putc(index, out);
}


void encodeRun(FILE *out, int runLength) {
    while (runLength > MAX_RUN_LENGTH) {
        putc(ZLE_MARKER, out);
        putc(MAX_RUN_LENGTH, out);
        runLength -= MAX_RUN_LENGTH;
    }
    putc(ZLE_MARKER, out);
    putc(runLength, out);
}


// Function to find an appropriate ZLE marker that is not present in the input file,
// or present with the smallest frequency
void find_zle_marker(const char *infile) {
    FILE *in = fopen(infile, "rb");
    if (!in) {
        perror("File error");
        exit(1);
    }

    int frequency[256] = {0};
    unsigned char ch;

    while (fread(&ch, 1, 1, in) == 1) {
        frequency[ch]++;
    }

    fclose(in);

    #pragma omp parallel for
    for (int i = 0; i < 256; i++) {
        if (frequency[i] == 0) {
            #pragma omp critical
            {
                if (ZLE_MARKER == 0) {  // Set ZLE_MARKER only once
                    ZLE_MARKER = i;
                }
            }
        }
    }

		/*if (ZLE_MARKER == 0) { // No zero-frequency byte was found
        // Find the byte with the lowest frequency
        int min_frequency = frequency[0];
        ZLE_MARKER = 0;

        for (int i = 1; i < 256; i++) {
            if (frequency[i] < min_frequency) {
                min_frequency = frequency[i];
                ZLE_MARKER = i;
            }
        }
    }*/


    printf("Selected ZLE_MARKER: 0x%02X\n", ZLE_MARKER);
}



void forward_mtf_with_zle(const char *infile, const char *outfile, int runLengthLimit) {
    find_zle_marker(infile);

    FILE *in = fopen(infile, "rb");
    FILE *out = fopen(outfile, "wb");
    if (!in || !out) {
        perror("File error");
        exit(1);
    }

    putc(ZLE_MARKER, out);

    int dict[256], map[256];
    for (int i = 0; i < 256; i++) {
        dict[i] = i;
        map[i] = i;
    }

    int lastIndex = -1, runLength = 0;
    unsigned char ch;
    while (fread(&ch, 1, 1, in) == 1) {
        int index = map[ch];
        if (index == 0) {
            runLength++;
            if (runLength == MAX_RUN_LENGTH) {
                encodeRun(out, runLength);
                runLength = 0;
            }
        } else {
            if (runLength > 0) {
                encodeRun(out, runLength);
                runLength = 0;
            }

            for (int check = index; check != 0; check--) {
                map[dict[check - 1]]++;
                dict[check] = dict[check - 1];
            }
            dict[0] = ch;
            map[ch] = 0;
            encodeSymbol(out, index);
        }
    }

    if (runLength > 0) {
        encodeRun(out, runLength);
    }

    flushOutput(out);
    fclose(in);
    fclose(out);
}


void inverse_mtf_with_zle(const char *infile, const char *outfile) {
    FILE *in = fopen(infile, "rb");
    FILE *out = fopen(outfile, "wb");
    if (!in || !out) {
        perror("File error");
        exit(1);
    }

    ZLE_MARKER = fgetc(in);

    int dict[256];
    for (int i = 0; i < 256; i++) {
        dict[i] = i;
    }

    unsigned char ch;
    while (fread(&ch, 1, 1, in) == 1) {
        if (ch == ZLE_MARKER) {
            unsigned char runLength;
						if (fread(&runLength, 1, 1, in) != 1) {
							perror("Error reading run length");
							fclose(in);
							fclose(out);
							exit(1);
						}
            for (int i = 0; i < runLength; i++) {
                putChar(out, dict[0]);
            }
        } else {
            int index = ch;
            int charVal = dict[index];
            putChar(out, charVal);

            for (int j = index; j > 0; j--) {
                dict[j] = dict[j - 1];
            }
            dict[0] = charVal;
        }
    }

    flushOutput(out);
    fclose(in);
    fclose(out);
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s -f|-i infile outfile\n", argv[0]);
        return 1;
    }

    char *mode = argv[1];
    char *infile = argv[2];
    char *outfile = argv[3];

    if (strcmp(mode, "-f") == 0) {
        forward_mtf_with_zle(infile, outfile, 10);
    } else if (strcmp(mode, "-i") == 0) {
        inverse_mtf_with_zle(infile, outfile);
    } else {
        fprintf(stderr, "Invalid mode. Use -f for forward or -i for inverse.\n");
        return 1;
    }

    return 0;
}

