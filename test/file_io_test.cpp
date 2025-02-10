#include "core/file/fileIO.hpp"
#include <iostream>

int main() {
    try {
        // Test JSON file IO
        std::string jsonFile = "test.json";
        auto jsonIO = FileIOFactory::createFileIO(jsonFile, "json");
        
        std::string jsonData = "{\"name\": \"Farid\", \"age\": 30}";
        jsonIO->write(jsonData);
        std::cout << "JSON Written: " << jsonData << std::endl;
        
        std::string jsonRead = jsonIO->read();
        std::cout << "JSON Read: " << jsonRead << std::endl;

        // Test Raw file IO
        std::string rawFile = "test.txt";
        auto rawIO = FileIOFactory::createFileIO(rawFile, "raw");
        
        std::string rawData = "Hello, File IO!";
        rawIO->write(rawData);
        std::cout << "Raw Data Written: " << rawData << std::endl;
        
        std::string rawRead = rawIO->read();
        std::cout << "Raw Data Read: " << rawRead << std::endl;
    } 
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
