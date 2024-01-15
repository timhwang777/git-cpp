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
#include <numeric>
#include <set>
#include <ctime>
#include "zlib_implement.h"

/*struct TreeEntry {
    std::string name;
    std::string mode;
    std::string type;
    std::string sha;
};*/

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

std::string compute_sha1(const std::string& data, bool print_out = false) {
    unsigned char hash[20]; // 160 bits long for SHA1
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }

    if (print_out)  {
        std::cout << ss.str() << std::endl;
    }

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

void compress_and_store (const std::string& hash, const std::string& content) {
    FILE* input = fmemopen((void*) content.c_str(), content.length(), "rb");
    std::string hash_folder = hash.substr(0, 2);
    std::string object_path = ".git/objects/" + hash_folder + '/';
    if (!std::filesystem::exists(object_path)) {
        std::filesystem::create_directories(object_path);
    }
    
    std::string object_file_path = object_path + hash.substr(2, 38);
    //std::cout << "object file path: " << object_file_path << std::endl;
    if (!std::filesystem::exists(object_file_path)) {
        FILE* output = fopen(object_file_path.c_str(), "wb");
        if (compress(input, output) != EXIT_SUCCESS) {
            std::cerr << "Failed to compress data.\n";
            return;
        }
        fclose(output);
    }

    fclose(input);
}

std::string hash_object(std::string filepath, std::string type = "blob", bool print_out = false) {
        // open the file
        std::ifstream inputFile(filepath, std::ios::binary);
        if(inputFile.fail()) {
            std::cerr << "Failed to open file.\n";
            return {};
        }

        // read the file
        std::string content(
            (std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>()
        );

        // create the content
        std::string header = type + " " + std::to_string(content.size());
        std::string file_content = header + '\0' + content;

        std::string hash = compute_sha1(file_content, false);

        /*
            todo: rewrite the the compress_and_store function
        */
        /*auto compressed_data = compress_data(file_content);
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
        object_file.close();*/

        compress_and_store(hash, file_content);
        inputFile.close();

        if (print_out) {
            std::cout << hash << std::endl;
        }

        return hash;
}

std::set<std::string> parse_tree_object (FILE* tree_object) {
    rewind(tree_object); // set the file position indicator to the beginning of the file
    
    std::vector<std::string> unsorted_directories;
    char mode[7];
    char filename[256];
    unsigned char hash[20];
    while (fscanf(tree_object, "%6s", mode) != EOF) {
        //std::cout << "mode:" << mode << '\n';
        // read the filename (up to the null byte)
        int i = 0;
        int c;
        while ((c = fgetc(tree_object)) != 0 && c != EOF) {
            // if the character is a blank space, continue
            if (c == ' ') {
                continue;
            }
            filename[i++] = c;
        }
        filename[i] = '\0'; // null-terminate the filename
        //std::cout << "filename:" << filename << '\n';

        // read the hash
        fread(hash, 1, 20, tree_object);

        unsorted_directories.push_back(filename);
    }

    std::sort(unsorted_directories.begin(), unsorted_directories.end()); // sort the directories lexicographically
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

/*
    todo: hash_digect, hash_object, compress_and_store
*/

std::string hash_digest (const std::string& input) {
    std::string condensed;

    for (size_t i = 0; i < input.length(); i += 2) {
        std::string byte_string = input.substr(i, 2);
        char byte = static_cast<char>(std::stoi(byte_string, nullptr, 16));

        condensed.push_back(byte);
    }

    return condensed;
}

std::string write_tree (const std::string& directory) {
    std::vector<std::string> tree_entries;
    std::vector<std::string> skip = {
        ".git", "server", "CMakeCache.txt", 
        "CMakeFiles", "Makefile", "cmake_install.cmake"
    };

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        std::string path = entry.path().string();
        //std::cout << "Entry path: " << path << '\n';
        
        if (std::any_of(skip.begin(), skip.end(), [&path](const std::string& s) {
            return path.find(s) != std::string::npos;
        })) {
            continue;
        }

        std::error_code ec;
        std::string entry_type = std::filesystem::is_directory(path, ec) ? "40000 " : "100644 ";
        std::string relative_path = path.substr(path.find(directory) + directory.length() + 1);
        std::string hash = std::filesystem::is_directory(path, ec) ?
                           hash_digest(write_tree(path.c_str())):
                           hash_digest(hash_object(path.c_str(), "blob", false));
        
        tree_entries.emplace_back(path + '\0' + entry_type + relative_path + '\0' + hash);
    }

    // sort the entries based on the absolute path
    std::sort(tree_entries.begin(), tree_entries.end());

    // delete the path from the beginning of each entry
    int bytes = 0;
    for (auto& entry : tree_entries) {
        entry = entry.substr(entry.find('\0') + 1);
        bytes += entry.length();
    }

    // concatenate the entries
    std::string tree_content = "tree " + std::to_string(bytes) + '\0';
    for (const auto& entry : tree_entries) {
        tree_content += entry;
    }

    // storing the tree object
    std::string tree_hash = compute_sha1(tree_content, false);
    compress_and_store(tree_hash.c_str(), tree_content);

    return tree_hash;
}

std::string commit_tree (std::string tree_sha, std::string parent_sha, std::string message) {
    std::string author = "John Doe <john.doe@gmail.com>";
    std::string committer = "John Doe <john.doe@gmail.com";
    std::string timestamp = std::to_string(std::time(nullptr));

    std::string commit_content = "tree " + tree_sha + "\n" +
                                 "parent " + parent_sha + "\n" +
                                 "author " + author + " " + timestamp + " -0800\n" +
                                 "committer " + committer + " " + timestamp + " -0800\n" +
                                 "\n" + message + "\n";

    std::string header = "commit " + std::to_string(commit_content.size()) + "\0";
    commit_content = header + commit_content;

    std::string commit_hash = compute_sha1(commit_content, false);
    compress_and_store(commit_hash.c_str(), commit_content);

    return commit_hash;
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
        std::string hash = hash_object(fileName, "blob", false);
        if (hash.empty()) {
            std::cerr << "Failed to hash object.\n";
            return EXIT_FAILURE;
        }

        std::cout << hash << std::endl;
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
    else if (command == "write-tree") {
        if (argc < 2) {
            std::cerr << "No command provided.\n";
            return EXIT_FAILURE;
        }

        std::filesystem::path current_path = std::filesystem::current_path();
        std::string tree_hash = write_tree(current_path.string());
        std::cout << tree_hash << std::endl;
    }
    else if (command == "commit-tree") {
        if (argc < 7) {
            std::cerr << "Too few arguments.\n";
            return EXIT_FAILURE;
        }

        std::string tree_sha = argv[2];
        std::string parent_sha = argv[4];
        std::string message = argv[6];
        
        std::string commit_hash = commit_tree(tree_sha, parent_sha, message);
        std::cout << commit_hash;
    }
    else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
