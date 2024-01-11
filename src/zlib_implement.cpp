#include <iostream>
#include <cstdlib>
#include <cstring>
#include <zlib.h>
#include "zlib_implement.h"

#define CHUNK 16384 //16KB

int decompress(FILE* input, FILE* output) {
    // initialize decompression stream
    z_stream stream = {0};
    if (inflateInit(&stream) != Z_OK) {
        std::cerr << "Failed to initialize decompression stream.\n";
        return EXIT_FAILURE;
    }

    // initialize decompression variables
    char in[CHUNK];
    char out[CHUNK];
    bool haveHeader = false;
    char header[64];
    int ret;

    do {
        stream.avail_in = fread(in, 1, CHUNK, input); // read from input file
        stream.next_in = reinterpret_cast<unsigned char*>(in); // set input stream
        if (ferror(input)) {
            std::cerr << "Failed to read from input file.\n";
            return EXIT_FAILURE;
        }
        if (stream.avail_in == 0) {
            break;
        }

        do {
            stream.avail_out = CHUNK; // set output buffer size
            stream.next_out = reinterpret_cast<unsigned char*>(out); // set output stream
            ret = inflate(&stream, Z_NO_FLUSH); // decompress
            if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                std::cerr << "Failed to decompress file.\n";
                return EXIT_FAILURE;
            }

            // write header to output file
            unsigned headerLen = 0, dataLen = 0;
            if (!haveHeader) {
                sscanf(out, "%s %u", header, &dataLen);
                haveHeader = true;
                headerLen = strlen(out) + 1;
            }
            // write decompressed data to output file
            if (dataLen > 0) {
                if(fwrite(out + headerLen, 1, dataLen, output) != dataLen) {
                    std::cerr << "Failed to write to output file.\n";
                    return EXIT_FAILURE;
                }
            }
        } while (stream.avail_out == 0);
        
    } while (ret != Z_STREAM_END);

    return inflateEnd(&stream) == Z_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

int compress(FILE* input, FILE* output) {
    // Initialize compression stream
    z_stream stream = {0};
    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        std::cerr << "Failed to initialize compression stream.\n";
        return EXIT_FAILURE;
    }

    char in[CHUNK];
    char out[CHUNK];
    int ret;
    int flush;

    do {
        stream.avail_in = fread(in, 1, CHUNK, input);
        stream.next_in = reinterpret_cast<unsigned char*>(in);
        if (ferror(input)) {
            (void)deflateEnd(&stream);  // Free memory
            std::cerr << "Failed to read from input file.\n";
            return EXIT_FAILURE;
        }
        flush = feof(input) ? Z_FINISH : Z_NO_FLUSH;

        do {
            stream.avail_out = CHUNK;
            stream.next_out = reinterpret_cast<unsigned char*>(out);
            ret = deflate(&stream, flush);
            if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                (void)deflateEnd(&stream);  // Free memory
                std::cerr << "Failed to compress file.\n";
                return EXIT_FAILURE;
            }
            size_t have = CHUNK - stream.avail_out;
            if (fwrite(out, 1, have, output) != have || ferror(output)) {
                (void)deflateEnd(&stream);  // Free memory
                std::cerr << "Failed to write to output file.\n";
                return EXIT_FAILURE;
            }
        } while (stream.avail_out == 0);
    } while (flush != Z_FINISH);

    // Clean up and check for errors
    if (deflateEnd(&stream) != Z_OK) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}