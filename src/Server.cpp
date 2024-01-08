#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstring>
#include <zlib.h> 
#include <vector>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

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

std::string compute_sha1(const std::string& data) {
    unsigned char hash[20];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }

    std::cout << ss.str() << std::endl;
    return ss.str();
}

std::vector<char> compress_data(const std::vector<char>& data) {
    std::vector<char> compressed_data;
    compressed_data.resize(compressBound(data.size()));
    uLongf compressed_size = compressed_data.size();

    if (compress(reinterpret_cast<Bytef*>(compressed_data.data()), &compressed_size,
        reinterpret_cast<const Bytef*>(data.data()), data.size()) != Z_OK) {
        std::cerr << "Failed to compress data.\n";
        return {};
    }
    compressed_data.resize(compressed_size);

    return compressed_data;
}

int hash_object(std::string filepath) {
        // open the file
        std::ifstream inputFile(filepath, std::ios::binary);
        if(inputFile.fail()) {
            std::cerr << "Failed to open file.\n";
            return EXIT_FAILURE;
        }

        // read the file
        std::vector<char> content(
            (std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>()
        );

        // create the header
        std::string header = "blob " + std::to_string(content.size()) + "\0";
        std::string file_content = header + "\0" + std::string(content.begin(), content.end());

        std::string hash = compute_sha1(file_content);
        auto compressed_data = compress_data(std::vector<char>(file_content.begin(), file_content.end()));
        if (compressed_data.empty()) {
            std::cerr << "Failed to compress data.\n";
            return EXIT_FAILURE;
        }

        // create the file path
        std::string object_dir = ".git/objects/" + hash.substr(0, 2);
        std::filesystem::create_directory(object_dir);

        std::string object_path = object_dir + "/" + hash.substr(2);
        std::ofstream object_file(object_path, std::ios::binary);
        if (object_file.fail()) {
            std::cerr << "Failed to create object file.\n";
            return EXIT_FAILURE;
        }
        object_file.write(compressed_data.data(), compressed_data.size());
        object_file.close();

        std::cout << hash << std::endl;

        return EXIT_SUCCESS;
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

        // create output file for standard output
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
    else if (command == "hash-object") {
        // check if file path is provided
        if (argc < 3) {
            std::cerr << "No file path provided.\n";
            return EXIT_FAILURE;
        }

        // retrieve file name
        char fileName[64];
        snprintf(fileName, sizeof(fileName), "%s", argv[3]);

        // hash the object
        if (hash_object(fileName) != EXIT_SUCCESS) {
            std::cerr << "Failed to hash object.\n";
            return EXIT_FAILURE;
        }
    }
    else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
