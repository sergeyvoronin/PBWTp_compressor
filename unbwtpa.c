#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BLOCK_SIZE 2097153  // The maximum block size handled

/* Inverse Burrows-Wheeler transform */

// init global arrays
unsigned char buffer[BLOCK_SIZE + 1];
unsigned int T[BLOCK_SIZE + 1], RunningTotal[257], Count[257];

size_t convert_to_bytes(const char *size_str) {
    char *end;
    double number = strtod(size_str, &end); // Extract numeric part
    printf("Number: %f\n", number); // Debugging log

    // Constants for units
    const size_t KILOBYTE = 1024;
    const size_t MEGABYTE = 1024 * 1024;
    const size_t GIGABYTE = 1024 * 1024 * 1024;

    // Identify the unit and calculate the size in bytes
    while (*end) { // Process all unit characters
        switch (*end++) {
            case 'K': case 'k':
                return (size_t)(number * KILOBYTE);
            case 'M': case 'm':
                return (size_t)(number * MEGABYTE);
            case 'G': case 'g':
                return (size_t)(number * GIGABYTE);
            case 'B': case 'b':
                break; // 'B' is just a marker for bytes, skip it
            default:
                fprintf(stderr, "Unknown unit: %c\n", *(end - 1));
                exit(EXIT_FAILURE);
        }
    }

    // If no unit is specified, assume it's just bytes
		return (size_t)round(number);
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input_file output_file block_size_str\n", argv[0]);
        return 1;
    }

    FILE *in_file = fopen(argv[1], "rb");
    if (!in_file) {
        fprintf(stderr, "Error opening input file: %s\n", argv[1]);
        return 1;
    }

    FILE *out_file = fopen(argv[2], "wb");
    if (!out_file) {
        fclose(in_file);
        fprintf(stderr, "Error opening output file: %s\n", argv[2]);
        return 1;
    }

		long nbytes = convert_to_bytes(argv[3]);
		printf("num bytes in block: %ld\n", nbytes);
		
    unsigned long buflen, first, last;

    while (fread(&buflen, sizeof(buflen), 1, in_file) == 1) {
			if (buflen > BLOCK_SIZE) {
					fprintf(stderr, "Buffer overflow detected! Buflen: %lu, BUFFER_SIZE: %d\n", buflen, BLOCK_SIZE);
					break;
			}

        if (fread(buffer, 1, buflen, in_file) != buflen) {
            fprintf(stderr, "Error reading buffer from input file.\n");
            break;
        }

        fread(&first, sizeof(first), 1, in_file);
        fread(&last, sizeof(last), 1, in_file);

        memset(Count, 0, sizeof(Count));
        for (unsigned int i = 0; i < buflen; ++i) {
            unsigned int index = (i == last) ? 256 : buffer[i];
            Count[index]++;
        }

        RunningTotal[0] = 0;
        for (int i = 1; i < 257; ++i) {
            RunningTotal[i] = RunningTotal[i - 1] + Count[i - 1];
        }

        memset(Count, 0, sizeof(Count));
        for (unsigned int i = 0; i < buflen; ++i) {
            unsigned int index = (i == last) ? 256 : buffer[i];
            T[RunningTotal[index] + Count[index]] = i;
            Count[index]++;
        }

        unsigned int pos = first;
        for (unsigned int i = 0; i < buflen - 1; ++i) {  // -1 because the last character is special
            putc(buffer[pos], out_file);
            pos = T[pos];
        }
    }

    fclose(in_file);
    fclose(out_file);
    return 0;
}

