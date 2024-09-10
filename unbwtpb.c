#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Computes and writes the Inverse Burrows-Wheeler Transform */

#define MAX_BLOCK_SIZE 8388608  // Maximum block size for N 

// Function to convert block size from a string to bytes
size_t convert_to_bytes(const char *size_str) {
    char *end;
    double number = strtod(size_str, &end); // Extract numeric part

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

    return (size_t)round(number);  // If no unit is specified, assume it's just bytes
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input_file output_file block_size_str\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];
    size_t block_size = convert_to_bytes(argv[3]);

    // Increase the block size to accommodate N+1 characters
    if (block_size > MAX_BLOCK_SIZE) {
        fprintf(stderr, "Block size exceeds maximum allowed size of %d bytes.\n", MAX_BLOCK_SIZE);
        return 1;
    }

    // Open the input and output files
    FILE *in_file = fopen(input_file, "rb");
    if (!in_file) {
        fprintf(stderr, "Error opening input file: %s\n", input_file);
        return 1;
    }

    FILE *out_file = fopen(output_file, "wb");
    if (!out_file) {
        fclose(in_file);
        fprintf(stderr, "Error opening output file: %s\n", output_file);
        return 1;
    }

    long buflen;
    unsigned long first, last;

    // Dynamically allocate memory for block processing
    // Allocate space for N+1 characters (hence block_size + 1)
    unsigned char* buffer = (unsigned char*) malloc(block_size + 2);  // +2 for safety
    unsigned int* T = (unsigned int*) malloc((block_size + 2) * sizeof(unsigned int));
    unsigned int* Count = (unsigned int*) malloc(257 * sizeof(unsigned int));
    unsigned int* RunningTotal = (unsigned int*) malloc(257 * sizeof(unsigned int));

    if (!buffer || !T || !Count || !RunningTotal) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    // Process each block in the input file sequentially
    while (fread(&buflen, sizeof(buflen), 1, in_file) == 1) {
        if (buflen > block_size + 1) {  // Allow buflen to be block_size + 1
            fprintf(stderr, "Buffer overflow detected! Buflen: %ld, Block size: %zu\n", buflen, block_size + 1);
            break;
        }

        // Read the block data
        if (fread(buffer, 1, buflen, in_file) != (size_t)buflen) {
            fprintf(stderr, "Error reading buffer from input file.\n");
            break;
        }

        // Read the first and last index
        fread(&first, sizeof(first), 1, in_file);
        fread(&last, sizeof(last), 1, in_file);

        // Initialize the Count array
        for (unsigned int i = 0; i < 257; i++) {
            Count[i] = 0;
        }

        // Count the occurrences of each byte
        for (unsigned int i = 0; i < (unsigned int)buflen; i++) {
            unsigned int index = (i == last) ? 256 : buffer[i];
            Count[index]++;
        }

        // Create the RunningTotal array
        unsigned int sum = 0;
        for (unsigned int i = 0; i < 257; i++) {
            RunningTotal[i] = sum;
            sum += Count[i];
            Count[i] = 0;  // Reset Count array for reuse
        }

        // Populate the transformation vector T
        for (unsigned int i = 0; i < (unsigned int)buflen; i++) {
            unsigned int index = (i == last) ? 256 : buffer[i];
            T[RunningTotal[index] + Count[index]] = i;
            Count[index]++;
        }

        // Write the output by following the T array
        unsigned int i = first;
        for (unsigned int j = 0; j < (unsigned int)(buflen - 1); j++) {
            putc(buffer[i], out_file);
            i = T[i];  // Follow the transformation vector
        }
    }

    // Free dynamically allocated memory
    free(buffer);
    free(T);
    free(Count);
    free(RunningTotal);

    fclose(in_file);
    fclose(out_file);
    return 0;
}
