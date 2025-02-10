#ifndef FILE_READER_HPP
#define FILE_READER_HPP

#include <string>
#include <memory>

// Abstract base class for file readers and writers
class FileIOBase {
public:
    virtual ~FileIOBase() = default;

    // Read the file and return its content as a string
    virtual std::string read() = 0;

    // Write content to the file
    virtual void write(const std::string& content) = 0;
};

// JSON file reader and writer
class JsonFileIO : public FileIOBase {
public:
    explicit JsonFileIO(const std::string& filePath);
    ~JsonFileIO() override;

    // Override read method for JSON files
    std::string read() override;

    // Override write method for JSON files
    void write(const std::string& content) override;

private:
    std::string filePath;
};

// Raw file reader and writer
class RawFileIO : public FileIOBase {
public:
    explicit RawFileIO(const std::string& filePath);
    ~RawFileIO() override;

    // Override read method for raw files
    std::string read() override;

    // Override write method for raw files
    void write(const std::string& content) override;

private:
    std::string filePath;
};

// FileIO Factory: Returns the appropriate reader and writer based on file format
class FileIOFactory {
public:
    // Factory method to create a reader and writer based on file format
    static std::unique_ptr<FileIOBase> createFileIO(const std::string& filePath, const std::string& format);
};

#endif // FILE_READER_HPP


//         std::unique_ptr<FileIOBase> fileIO = FileIOFactory::createFileIO(filePath, format);
