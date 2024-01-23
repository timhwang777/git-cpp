/*struct TreeEntry {
    std::string name;
    std::string mode;
    std::string type;
    std::string sha;
};*/

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

int cat_file_tree(const char* file_path, const std::string& dir, FILE* dest, bool print_out = false) {
    try {
        std::string blob_sha = file_path;
        std::string blob_path = dir + "/.git/objects/" + blob_sha.insert(2, "/");
        if (print_out) std::cout << "blob path: " << blob_path << std::endl;
        FILE* blob_file = fopen(blob_path.c_str(), "rb");
        if (blob_file == NULL) {
            std::cerr << "Invalid object hash.\n";
            return EXIT_FAILURE;
        }

        decompress(blob_file, dest);
        fclose(blob_file);
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

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

else if (object_type == 7) { // reference deltas
    // process reference deltas
    std::string delta_hash_digest = pack.substr(current_position, 20);
    std::string base_object_hash = digest_to_hash(delta_hash_digest);
    current_position += 20;

    // read the base object's contents
    std::ifstream base_object_file(dir + "/.git/objects/" + base_object_hash.insert(2, "/"));
    std::stringstream buffer;
    buffer << base_object_file.rdbuf();
    std::string base_object_contents = decompress_string(buffer.str());
    base_object_contents = base_object_contents.substr(base_object_contents.find('\0') + 1); // remove the hash

    // apply delta to base object
    std::string delta_contents = decompress_string(pack.substr(current_position));
    std::string reconstructed_contents = apply_delta(delta_contents, base_object_contents);

    // update master commits if hash matches
    if (base_object_hash.compare(packhash) == 0) {
        master_commit_contents = reconstructed_contents;
    }

    // advance position past the delta data
    current_position += compress_string(delta_contents).length();
}