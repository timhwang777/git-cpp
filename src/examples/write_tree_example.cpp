
std::string gethash(std::string fileContentHeader) {
    unsigned char digest[20];
    SHA1((const unsigned char*)fileContentHeader.c_str(), fileContentHeader.length(), digest);
    char hash[41];
    for (int i = 0; i < 20; i++) {
        sprintf(hash + i * 2, "%02x", (unsigned char)digest[i]);
    }
    hash[40] = '\0';

    return std::string(hash);
}

void store(char* hash, std::string fileContentHeader) {
    FILE* source = fmemopen((void *)fileContentHeader.c_str(), fileContentHeader.length(), "r");
    std::string folder(hash, 2);
    folder = ".git/objects/" + folder + '/';
    if (!std::filesystem::exists(folder)) {
        std::filesystem::create_directories(folder);
    }
    std::string tmp = hash;
    FILE* dest = fopen((folder + std::string({hash + 2, 39})).c_str(), "w");
    def(source, dest, COMPRESSIONLEVEL);
    std::string destpath = folder + std::string({hash + 2, 39});
    if (! std::filesystem::exists(destpath)) {
        FILE* dest = fopen(destpath.c_str(), "w");
        def(source, dest, COMPRESSIONLEVEL);
        fclose(dest);
    }
    
    fclose(source);
}

std::string hashobject(char* filepath, std::string type) {
    std::ifstream inputFile(filepath);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << filepath << std::endl;
        return NULL;
    }
    std::string fileContent((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    int file_size = fileContent.length();
    std::string header = type + " " + std::to_string(file_size) + '\0';
    std::string fileContentHeader = header + fileContent;
    
    std::string hash = gethash(fileContentHeader);
    store((char*)hash.c_str(), fileContentHeader);
    inputFile.close();
    fclose(source);
    fclose(dest);
    
    return hash;
}

std::string hashtodigest(std::string input) {
    std::string condensed;
    
    for (size_t i = 0; i < input.length(); i += 2) {
        std::string byteString = input.substr(i, 2);
        char byte = static_cast<char>(std::stoi(byteString, nullptr, 16));
        
        condensed.push_back(byte);
    }
    
    return condensed;
}


std::string writetree(std::string filepath, int verbose) {
    std::ostringstream fileContent;
    std::string path;
    std::vector<std::string> lines;
    for (const auto & entry : std::filesystem::directory_iterator(filepath)) {
        path = entry.path();
        if (path.compare("./.git") == 0) {
            continue;
        }
        if (path.compare("./server") == 0) {
            continue;
        }
        if (path.compare("./CMakeCache.txt") == 0) {
            continue;
        }
        if (path.compare("./CMakeFiles") == 0) {
            continue;
        }
        if (path.compare("./Makefile") == 0) {
            continue;
        }
        if (path.compare("./cmake_install.cmake") == 0) {
            continue;
        }
        // std::cout << path << "\n";
        // std::cout << path.substr(path.find(filepath) + filepath.length() + 1) << "\n";
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec)) {
            // fileContent << "40000 " << path.substr(path.find(filepath) + filepath.length() + 1) << '\0' << hashtodigest(writetree((char*)path.c_str()));
            lines.push_back(path + '\0' + "40000 " + path.substr(path.find(filepath) + filepath.length() + 1) + '\0' + hashtodigest(writetree((char*)path.c_str(), 0)));
        } else {
            // fileContent << "100644 " << path.substr(path.find(filepath) + filepath.length() + 1) << '\0' << hashtodigest(hashobject((char*)path.c_str(), "blob"));
            lines.push_back(path + '\0' + "100644 " + path.substr(path.find(filepath) + filepath.length() + 1) + '\0' + hashtodigest(hashobject((char*)path.c_str(), "blob")));
        }
    }
    int bytes = 0;
    std::sort(lines.begin(), lines.end());
    // if (lines.size() > 4) {
        // lines.erase(lines.begin(), lines.begin() + 4);
    // }
    for (int i = 0; i < lines.size(); i++) {
        lines[i] = lines[i].substr(lines[i].find('\0') + 1);
        bytes += lines[i].length();
        std::cerr << lines[i].substr(0, lines[i].find('\0')) << "\n";
        
    }
    lines.insert(lines.begin(), "tree " + std::to_string(bytes) + '\0');
    std::string contentStr = std::accumulate(lines.begin(), lines.end(), std::string());
    // std::ostringstream header;
    // header << "tree " << fileContent.str().length() << '\0';
    // std::string contentStr = header.str() + fileContent.str();
    std::string hash = gethash(contentStr);
    store((char*)hash.c_str(), contentStr);
    return hash;
}

else if (command == "write-tree") {
    if (argc != 2) {
        std::cerr << "Too many arguments.\n";
    }
    std::cout << writetree(".", 1) << "\n";

    return EXIT_SUCCESS;
}