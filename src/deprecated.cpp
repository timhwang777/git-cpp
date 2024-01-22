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