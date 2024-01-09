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
#include <algorithm>
#include <set>

#include "zlib_implement.h"

/* Functions */
int cat_file(const char* filepath) {
        FILE* dataFile = fopen(filepath, "rb");
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

        return EXIT_SUCCESS;
}

std::string compute_sha1(const std::string& data) {
    unsigned char hash[20]; // 160 bits long for SHA1
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }

    std::cout << ss.str() << std::endl;
    return ss.str();
}

std::vector<char> compress_data(const std::string& data) {
    unsigned long len = data.size();
    unsigned long compressed_len = compressBound(len); // returns the maximum length of the compressed data from zlib
    std::vector<char> compressed_data(compressed_len);

    if(compress(reinterpret_cast<Bytef*>(compressed_data.data()), &compressed_len, reinterpret_cast<const Bytef*>(data.c_str()), len) != Z_OK) {
        std::cerr << "Failed to compress data.\n";
        return {};
    }

    compressed_data.resize(compressed_len);
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
        std::string content(
            (std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>()
        );

        // create the header
        std::string header = "blob " + std::to_string(content.size());
        std::string file_content = header + '\0' + content;

        std::string hash = compute_sha1(file_content);
        auto compressed_data = compress_data(file_content);
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

std::set<std::string> parse_tree_object (FILE* tree_object) {
    rewind(tree_object);
    
    std::vector<std::string> unsorted_directories;
    char mode[7];
    char filename[256];
    unsigned char hash[20];
    while (fscanf(tree_object, "%6s", mode) != EOF) {
        std::cout << "mode:" << mode << '\n';
        // read the filename (up to the null byte)
        int i = 0;
        int c;
        while ((c = fgetc(tree_object)) != 0 && c != EOF) {
            filename[i++] = c;
        }
        filename[i] = '\0'; // null-terminate the filename
        std::cout << "filename:" << filename << '\n';

        // read the hash
        fread(hash, 1, 20, tree_object);

        // if the mode is "40000", add the filename to the set of directories
        if (strcmp(mode, "40000") == 0) {
            unsorted_directories.push_back(filename);
        }
    }

    std::sort(unsorted_directories.begin(), unsorted_directories.end()); // sort the directories (lexicographically
    std::set<std::string> sorted_directories(unsorted_directories.begin(), unsorted_directories.end()); // remove duplicates

    return sorted_directories;
}

int ls_tree (const char* object_hash) {
    // retrieve the object path
    char object_path[64];
    snprintf(object_path, sizeof(object_path), ".git/objects/%.2s/%s", object_hash, object_hash + 2);

    // set the input and output file descriptors
    FILE* object_file = fopen(object_path, "rb");
    if(object_file == NULL) {
        std::cerr << "Invalid object hash.\n";
        return EXIT_FAILURE;
    }
    FILE* output_file = tmpfile();
    if(output_file == NULL) {
        std::cerr << "Failed to create output file.\n";
        return EXIT_FAILURE;
    }

    if(decompress(object_file, output_file) != EXIT_SUCCESS) {
        std::cerr << "Failed to decompress object file.\n";
        return EXIT_FAILURE;
    }

    std::set<std::string> directories = parse_tree_object(output_file);

    // print the directories
    for (const std::string& directory : directories) {
        std::cout << directory << '\n';
    }

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
        if (cat_file(dataPath) != EXIT_SUCCESS) {
            std::cerr << "Failed to retrieve object.\n";
            return EXIT_FAILURE;
        }
    }
    else if (command == "hash-object") {
        // check if file path is provided
        if (argc < 4) {
            std::cerr << "No file path provided.\n";
            return EXIT_FAILURE;
        }

        // retrieve file name
        std::string fileName = argv[3];

        // hash the object
        if (hash_object(fileName) != EXIT_SUCCESS) {
            std::cerr << "Failed to hash object.\n";
            return EXIT_FAILURE;
        }
    }
    else if (command == "ls-tree") {
        if (argc < 4) {
            std::cerr << "No object hash provided.\n";
            return EXIT_FAILURE;
        }

        // retrieve file path and check if object hash is valid
        std::string objectHash = argv[3];
        if (ls_tree(objectHash.c_str()) != EXIT_SUCCESS) {
            std::cerr << "Failed to retrieve object.\n";
            return EXIT_FAILURE;
        }
    }
    else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
