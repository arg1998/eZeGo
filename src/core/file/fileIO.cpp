#include "fileIO.hpp"
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

// JsonFileIO Implementation
JsonFileIO::JsonFileIO(const std::string& filePath) : filePath(filePath) {}
JsonFileIO::~JsonFileIO() = default;

std::string JsonFileIO::read() {
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    json j;
    file >> j;
    return j.dump(4); // Pretty-print JSON
}

void JsonFileIO::write(const std::string& content) {
    std::ofstream file(filePath);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    json j = json::parse(content);
    file << j.dump(4); // Pretty-print JSON
}
// RawFileIO Implementation
RawFileIO::RawFileIO(const std::string& filePath) : filePath(filePath) {}
RawFileIO::~RawFileIO() = default;

std::string RawFileIO::read() {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void RawFileIO::write(const std::string& content) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    file.write(content.data(), content.size());
}

// FileIOFactory Implementation
std::unique_ptr<FileIOBase> FileIOFactory::createFileIO(const std::string& filePath, const std::string& format) {
    if (format == "json") {
        return std::make_unique<JsonFileIO>(filePath);
    } else if (format == "raw") {
        return std::make_unique<RawFileIO>(filePath);
    } else {
        throw std::invalid_argument("Unsupported file format: " + format);
    }
}
