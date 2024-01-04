#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstring>
#include <zlib.h> 

#define CHUNK 16384 //16KB

/* Functions */
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
            unsigned headerLen = 0;
            if (!haveHeader) {
                headerLen = CHUNK - stream.avail_out; // get header size
                std::cout << headerLen << " " << stream.avail_out << std::endl;
                if(headerLen > 0) {
                    haveHeader = true;
                    memcpy(header, out, headerLen);
                    if(fwrite(header, 1, headerLen, output) != headerLen) {
                        std::cerr << "Failed to write header to output file.\n";
                        return EXIT_FAILURE;
                    }
                }
            }

            // write decompressed data to output file
            if (stream.avail_out < CHUNK) {
                unsigned dataLen = CHUNK - headerLen - stream.avail_out;
                if(fwrite(out + headerLen, 1, dataLen, output) != dataLen) {
                    std::cerr << "Failed to write decompressed data to output file.\n";
                    return EXIT_FAILURE;
                }
            }
        } while (stream.avail_out == 0);
        
    } while (ret != Z_STREAM_END);

    return inflateEnd(&stream) == Z_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }

    std::string command = argv[1];

    if (command == "init") {
        try {
            std::filesystem::create_directory(".git");
            std::filesystem::create_directory(".git/objects");
            std::filesystem::create_directory(".git/refs");

            std::ofstream headFile(".git/HEAD");
            if (headFile.is_open()) { // create .git/HEAD file
                headFile << "ref: refs/heads/master\n"; // write to the headFile
                headFile.close();
            } else {
                std::cerr << "Failed to create .git/HEAD file.\n";
                return EXIT_FAILURE;
            }
           
            std::cout << "Initialized git directory\n";
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    else if (command == "cat-file") {
        // check if object hash is provided
        if (argc < 3) {
            std::cerr << "No object hash provided.\n";
            return EXIT_FAILURE;
        }

        // retrieve file path and check if object hash is valid
        char dataPath[64];
        snprintf(dataPath, sizeof(dataPath), ".git/objects/%.2s/%s", argv[3], argv[3] + 2);
        FILE* dataFile = fopen(dataPath, "rb");
        if (!dataFile) {
            std::cerr << "Invalid object hash.\n";
            return EXIT_FAILURE;
        }

        // create output file
        FILE* outputFile = fdopen(1, "wb");
        if (!outputFile) {
            std::cerr << "Failed to create output file.\n";
            return EXIT_FAILURE;
        }

        // decompress data file
        if (decompress(dataFile, outputFile) != EXIT_SUCCESS) {
            std::cerr << "Failed to decompress data file.\n";
            return EXIT_FAILURE;
        }
    }
    else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
