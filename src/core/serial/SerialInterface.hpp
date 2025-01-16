#include "core/definitions.hpp"

#include <string>
struct SerialPortInfo {
    std::string port_name;
    std::string description;
    std::string hardware_id;
};

struct SerialPortConfig {
    std::string port_name;
    u32 baud_rate;
    u8 character_size = 8;
    u8 stop_bits = 0; // 0 = one, 1 = onepointfive, 2 = two
    u8 parity = 0;    // 0 = none, 1 = odd, 2 = even
};

struct HardwareInterfaceInfo {
    std::string name;
    std::string memory;
    std::string hardware_id;
};

struct HardwareInterfaceState {
    std::string device;
    bool connected;
    bool initialized;

};

void initiliazeSerialIO();
void shutdownSerialIO();
void openIOService(const SerialPortConfig& config);

void getSerialPorts(std::vector<SerialPortInfo>& ports);

// TODO: (FARID)What is our data representation type?
void sendSerialDataSync(const SerialPortConfig& config, const std::vector<u8>& data);
void sendSerialDataAsync(const SerialPortConfig& config, const std::vector<u8>& data);

void receiveSerialDataSync(const SerialPortConfig& config, std::vector<u8>& data);
void receiveSerialDataAsync(const SerialPortConfig& config, std::vector<u8>& data);
void runBenchmarkIO(const SerialPortConfig& config, const u32& data_size, const u32& iterations);
