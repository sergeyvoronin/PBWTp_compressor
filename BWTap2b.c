//
//  BWTap2.c
//  Sergey Voronin
//  Parallelized Burrows Wheeler transform applicable to general data based on Mark Nelson's 1996 code
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#if !defined( unix )
#include <io.h>
#endif
#include <math.h>
#include <limits.h>

//#define BLOCK_SIZE 20000
//#define NUM_THREADS 16 
#define MAX_THREADS 8

// Structure to hold block data
typedef struct {
    unsigned char *buff; // len BLOCK_SIZE
    size_t size;
		int bnum;
		int *inds; // len BLOCK_SIZE + 1
} BlockData;


//
// length has the number of bytes presently read into the buffer,
// buffer contains the data itself.  indices[] is an array of
// indices into the buffer.  indices[] is what gets sorted in
// order to sort all the strings in the buffer.
//
long length;
//int indices[ BLOCK_SIZE + 1 ];
pthread_key_t buffer_key;

int memcmp_signed;

int active_threads = 0;
pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

/* The function iterates through the memory blocks one byte at a time. It compares the corresponding bytes from each block until it finds a mismatch or reaches the end of the blocks.
 * It casts the pointers to unsigned char*, meaning each byte is treated as an unsigned character. This is crucial for binary data, where the sign of the byte should not influence the comparison.
 * */
int unsigned_memcmp( void *p1, void *p2, unsigned int i )
{
    unsigned char *pc1 = (unsigned char *) p1;
    unsigned char *pc2 = (unsigned char *) p2;
    while ( i-- ) {
        if ( *pc1 < *pc2 )
            return -1;
        else if ( *pc1++ > *pc2++ )
            return 1;
    }
    return 0;
}

//
// This is the special comparison function used when calling
// qsort() to sort the array of indices into the buffer. Remember that
// the character at buffer+length doesn't really exist, but it is assumed
// to be the special end-of-buffer character, which is bigger than
// any character found in the input buffer.  So I terminate the
// comparison at the end of the buffer.
//

/* The primary purpose of the bounded_compare function is to compare two rotations within the 'indices' array based on their cyclic order within a block of the input data. It ensures that the rotations are sorted in a way that groups similar characters together while maintaining their relative order within each rotation.
Initialization:

    It takes two pointers to unsigned integers (i1 and i2) as parameters. These pointers represent the positions of two rotations within the 'indices' array. These positions indicate where each rotation begins in the original data.
    It then calculates the lengths of the two rotations (l1 and l2) based on their positions within the array and the total length of the data (length). */
int
#if defined( _MSC_VER )
_cdecl
#endif
bounded_compare( const unsigned int *i1,
                 const unsigned int *i2 )
{

    // get block data specific to the calling thread, need access to the buffer and its size
    BlockData *bdata = (BlockData *)pthread_getspecific(buffer_key);
    unsigned char *lbuffer = bdata->buff;

    // Calculate the remaining length for each index
    // l1 and l2 are calculated as the lengths from the indices *i1 and *i2 to the end of the buffer. These represent the remaining lengths of the rotations being compared.
    unsigned int l1 = (unsigned int)(bdata->size - *i1);
    unsigned int l2 = (unsigned int)(bdata->size - *i2);
    // Set min_length to the smaller of l1 and l2. This is the maximum length up to which the two rotations can be compared without exceeding the buffer.
    unsigned int min_length = (l1 < l2) ? l1 : l2;
    int result;

    // default to using memcmp, adjust to use unsigned_memcmp as necessary if special input handling is neededed
    /*if ( memcmp_signed )
        result = unsigned_memcmp( lbuffer + *i1,
                                  lbuffer + *i2,
                                  min_length );
    else
        result = memcmp( lbuffer + *i1,
                         lbuffer + *i2,
                         min_length );*/

    /*  The memcmp() function is used to compare the two substrings of the buffer starting at indices *i1 and *i2, up to min_length bytes. The result of this comparison is stored in result.
    If result is non-zero, it means a difference was found within the first min_length bytes, and result will be negative if the substring starting at *i1 is lexicographically smaller, and positive if it's larger. */
    result = memcmp( lbuffer + *i1, lbuffer + *i2, min_length );

    /* If result is zero, it means the substrings are equal up to min_length. In this case, the function returns l2 - l1, which effectively compares the lengths of the remaining
     parts of the rotations. This ensures that shorter rotations are considered smaller in the sorting order. If result is non-zero, it's returned directly, as it already
     indicates the correct ordering based on the substring comparison. */
    if ( result == 0 )
        return l2 - l1;
    else
        return result;
};


/* The primary purpose of the bounded_compare function is to compare two rotations within the 'indices' array based on their cyclic order within a block of the input data. It ensures that the rotations are sorted in a way that groups similar characters together while maintaining their relative order within each rotation.
Initialization:

    It takes two pointers to unsigned integers (i1 and i2) as parameters. These pointers represent the positions of two rotations within the 'indices' array. These positions indicate where each rotation begins in the original data.
    It then calculates the lengths of the two rotations (l1 and l2) based on their positions within the array and the total length of the data (length).

Character Comparison:

    It enters a loop that iterates from i = 0 up to the minimum of l1 and l2.
    Within the loop, it compares characters in the buffer at positions (*i1 + i) and (*i2 + i) for each i. If the characters are different, it returns -1 if the character in rotation i1 is smaller, and 1 if it's larger.

Prefix Equality Check:

    If the loop completes without finding a difference in characters, it means the rotations have identical prefixes up to the length of the shorter rotation.
    In this case, it compares the lengths l1 and l2 to determine the order. If l1 is shorter than l2, it returns 1, indicating that rotation i1 should come after rotation i2. If l1 is longer than l2, it returns -1, indicating the opposite.
    If the lengths are equal, it returns 0 to indicate that the rotations are identical.
 */

int bounded_compare1(const unsigned int *i1, const unsigned int *i2)
{
    // get block data specific to the calling thread
    BlockData *bdata = (BlockData *)pthread_getspecific(buffer_key);
    unsigned char *lbuffer = bdata->buff;

    // Calculate the remaining length for each index
    unsigned int l1 = (unsigned int)(bdata->size - *i1);
    unsigned int l2 = (unsigned int)(bdata->size - *i2);
    unsigned int i;

    // Compare the characters in the buffer at the specified positions
    for (i = 0; i < l1 && i < l2; i++) {
        // Compare the characters at the current positions
        if (lbuffer[*i1 + i] < lbuffer[*i2 + i]) {
            // If the character at index i1 is less, return -1
            return -1;
        } else if (lbuffer[*i1 + i] > lbuffer[*i2 + i]) {
            // If the character at index i1 is greater, return 1
            return 1;
        }
    }

    // If we reach this point, it means the prefixes are equal up to the length of the shorter rotation
    // In this case, we compare the lengths to determine the order
    if (l1 < l2) {
        return 1;
    } else if (l1 > l2) {
        return -1;
    } else {
        return 0;
    }
}


void *process_block(void *arg) {
    BlockData *bdata = (BlockData *)arg;
    int i; long len; // do not use globals to avoid race conditions

    // Associate the local buffer with the key for this thread
    pthread_t thread_id = pthread_self();
    pthread_setspecific(buffer_key, bdata);
		printf("Block num: %d, Thread ID: %lu\n", bdata->bnum, (unsigned long) thread_id);

    // init indices and sort with current block data
    len = bdata->size;
    printf("Block num: %d, length = %ld\n", bdata->bnum, len);
    for (i = 0 ; i <= len ; i++){
      bdata->inds[ i ] = i;
    }

		// perform sort with custom compare function
		qsort( bdata->inds,
               (int)( len + 1 ),
               sizeof( int ),
               ( int (*)(const void *, const void *) ) bounded_compare );
        fprintf( stderr, "\n" );

    pthread_exit(NULL);
}


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

    //return (size_t)number; // If no unit is specified, assume it's just bytes
		return (size_t)round(number);
}


int main( int argc, char *argv[] )
{
		if(argc != 4) {
        fprintf(stderr, "Usage: %s input_file output_file block_size\n", argv[0]);
        return 1;
    }

    printf("starting up..\n");

    char in_file[200], out_file[200], nthreads_str[20];
    unsigned char *buffer;
    int i, nb, nblocks = 0, debug = 0, max_threads = 8;
    long l, lSize, first, last, totsize = 0;
    size_t current_offset = 0; // Track the current file position
    size_t block_start, block_end; // Track the start and end bytes of each block

    strcpy(in_file, argv[1]);
    strcpy(out_file, argv[2]);
    size_t BLOCK_SIZE = convert_to_bytes(argv[3]);
    printf("Size in bytes for block read: %zu\n", BLOCK_SIZE);
    //strcpy(nthreads_str, argv[4]);
    buffer = (unsigned char*)malloc(BLOCK_SIZE);

    FILE *fp_in, *fp_out, *fp_log;
    printf("opening %s and %s\n", in_file, out_file);
    fp_in = fopen(in_file, "rb");
    fp_out = fopen(out_file, "wb");
    fp_log = fopen("temp/bwt_log.txt", "w");

#if !defined( unix )
    setmode( fileno( fp_in ), O_BINARY );
    setmode( fileno( fp_out ), O_BINARY );
#endif
    /* In hexadecimal, \x070 is 0x70 and \x080 is 0x80. The result of this comparison tells you whether memcmp() is treating these bytes as signed or unsigned values.
    If it treats them as unsigned, \x080 (128 in decimal) is greater than \x070 (112 in decimal), so memcmp() will return a positive value.
    If it treats them as signed, \x080 is interpreted as -128 (in a typical two's complement system), which is less than \x070 (112), so memcmp() will return a negative value.*/
    if ( memcmp( "\x070", "\x080", 1 ) < 0 ) {
        memcmp_signed = 0;
        fprintf( stderr, "memcmp() treats character data as unsigned\n" );
    } else {
        memcmp_signed = 1;
        fprintf( stderr, "memcmp() treats character data as signed\n" );
    }

    // Create the thread-specific data key
    if (pthread_key_create(&buffer_key, NULL)) {
        fprintf(stderr, "Error creating pthread key\n");
            return 1;
    }

    fseek(fp_in, 0L, SEEK_END);
    lSize = ftell(fp_in);
    rewind(fp_in);

    // Calculate the number of blocks based on file size and block size
    nblocks = (int)((lSize + BLOCK_SIZE - 1) / BLOCK_SIZE);
    printf("nblocks = %d\n", nblocks);

    // Initialize variables
    BlockData* blocks = (BlockData*)malloc((nblocks+1) * sizeof(BlockData)); // Array to hold block data


    // Fill blocks array with input data
        nblocks = 0;
        for ( ; ; ) {
            //length = fread( (char *) blocks[nblocks].buff, 1, BLOCK_SIZE, fp_in);
            length = fread( buffer, 1, BLOCK_SIZE, fp_in);
            if ( length == 0 )
                break;
            blocks[nblocks].size = length;
            blocks[nblocks].buff = (unsigned char*)malloc(length*sizeof(unsigned char));
            // The +1 accounts for the extra index needed to represent the virtual end-of-buffer character
            blocks[nblocks].inds = (int*)malloc((length+1)*sizeof(int));
            //strcpy(blocks[nblocks].buff, buffer);
            memcpy(blocks[nblocks].buff, buffer, length); // Replaced strcpy with memcpy
            nblocks++;
        }

        fclose(fp_in);
        printf("Read data for %d blocks..\n", nblocks);
        pthread_t threads[nblocks];

        // now peform BWT on each block in parallel
        for(nb = 0; nb < nblocks; nb++){
            memcpy(buffer, blocks[nb].buff, blocks[nb].size);
            length = blocks[nb].size;
            blocks[nb].bnum = nb+1;

            // Create the thread and pass the structure as an argument
            fprintf( stderr, "Performing BWT on %ld bytes (block # %d) with thread\n", blocks[nb].size, blocks[nb].bnum);
            if (pthread_create(&threads[nb], NULL, process_block, (void *)&blocks[nb])){
                fprintf(stderr, "Error creating thread\n"); return 1;
            }
            if(nb > 0 && (nb % max_threads == 0)){
                sleep(2);
            }
        }

    // Wait for threads to finish, then delete key
    for (nb = 0; nb < nblocks; nb++) {
        pthread_join(threads[nb], NULL);
    }
    pthread_key_delete(buffer_key);

    // Write out the results
    printf("Writing data to %s\n", out_file);
    current_offset = 0;

    for (nb = 0; nb < nblocks; nb++) {
        l = blocks[nb].size + 1;
        block_start = current_offset;

        // Write the block size
        fwrite( (char *) &l, 1, sizeof( long ), fp_out );
        current_offset += sizeof(long);

        for ( i = 0 ; i < l ; i++ ) {
            if ( blocks[nb].inds[i] == 1 )
                first = i;
            if ( blocks[nb].inds[i] == 0 ) {
                last = i;
                fputc( '?', fp_out );
                current_offset += 1; // One character written
            } else{
                fputc( blocks[nb].buff[ blocks[nb].inds[i] - 1 ], fp_out );
                current_offset += 1; // One character written
            }
        }
        fprintf( stderr,
            "first = %ld"
            "  last = %ld\n",
            first,
            last );
        fwrite( (char *) &first, 1, sizeof( long ), fp_out );
        fwrite( (char *) &last, 1, sizeof( long ), fp_out );
        current_offset += sizeof(long) * 2; // Two longs written

        // Record the end of the block
        size_t block_end = current_offset;

        fprintf(fp_log, "Block %d: Start = %zu, End = %zu, first = %ld, last = %ld\n",
                nb, block_start, block_end, first, last);
    }

    fclose(fp_out);
    fclose(fp_log);
    return 0;
}

