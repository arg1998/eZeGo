#include "SerialInterface.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include "core/logger/logger.hpp"

// This is a simple serial IO implementation using Boost.Asio
// It provides synchronous and asynchronous read and write operations
// for serial ports.

// Static variables for Boost.Asio service
static std::shared_ptr<boost::asio::io_context> io_service;
static std::shared_ptr<boost::asio::serial_port> serial_port;


boost::asio::serial_port_base::stop_bits convertStopBits(u8 stop_bits);
boost::asio::serial_port_base::parity convertParity(u8 parity);
void initiliazeSerialIO() {
    EZ_LOG_TRACE();
    io_service = std::make_shared<boost::asio::io_context>();
    
}

 
void shutdownSerialIO() {
    EZ_LOG_TRACE();
    if (serial_port && serial_port->is_open()) {
        serial_port->close();
    }
    io_service.reset();
}

void openIOService(const SerialPortConfig& config) {
    EZ_LOG_TRACE();
    if (!io_service) {
        throw std::runtime_error("Serial IO not initialized.");
    }
    try {
        serial_port = std::make_shared<boost::asio::serial_port>(*io_service);
        serial_port->open(config.port_name);

        serial_port->set_option(boost::asio::serial_port_base::baud_rate(config.baud_rate));
        serial_port->set_option(boost::asio::serial_port_base::character_size(config.character_size));
        serial_port->set_option(convertStopBits(config.stop_bits));
        serial_port->set_option(convertParity(config.parity));
    } catch (const std::exception& e) {
        throw std::runtime_error("Error opening IO service: " + std::string(e.what()));
    }

}

boost::asio::serial_port_base::stop_bits convertStopBits(u8 stop_bits) {
    using StopBits = boost::asio::serial_port_base::stop_bits;
    switch (stop_bits) {
        case 0: return StopBits(StopBits::one);
        case 1: return StopBits(StopBits::onepointfive);
        case 2: return StopBits(StopBits::two);
        default: throw std::invalid_argument("Invalid stop_bits value");
    }
}

boost::asio::serial_port_base::parity convertParity(u8 parity) {
    using Parity = boost::asio::serial_port_base::parity;
    switch (parity) {
        case 0: return Parity(Parity::none);
        case 1: return Parity(Parity::odd);
        case 2: return Parity(Parity::even);
        default: throw std::invalid_argument("Invalid parity value");
    }
}

// TODO: (FARID) Implement this function it should give back the available serial ports
void getSerialPorts(std::vector<SerialPortInfo>& ports) {
    EZ_LOG_TRACE();

}

void sendSerialDataSync(const SerialPortConfig& config, const std::vector<u8>& data) {
    EZ_LOG_TRACE();
    if (!io_service) {
        throw std::runtime_error("Serial IO not initialized.");
    }
    try {
        if (data.empty()) {
            throw std::invalid_argument("Data vector is empty.");
        }

        // Send the entire buffer
        boost::asio::write(*serial_port, boost::asio::buffer(data));

    } catch (const std::exception& e) {
        throw std::runtime_error("Error sending serial data: " + std::string(e.what()));
    }
}


void sendSerialDataAsync(const SerialPortConfig& config, const std::vector<u8>& data) {
    EZ_LOG_TRACE();
    if (!io_service) {
        throw std::runtime_error("Serial IO not initialized.");
    }

    try {
        // Create a shared pointer to the data to ensure its lifetime is managed during async operation
        auto write_buffer = std::make_shared<std::vector<u8>>(data);

        // Asynchronously write the buffer
        boost::asio::async_write(*serial_port, boost::asio::buffer(*write_buffer),
                                 [write_buffer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                                     if (!ec) {
                                         std::cout << "Asynchronously sent " << bytes_transferred
                                                   << " bytes of data." << std::endl;
                                     } else {
                                         std::cerr << "Error during async write: " << ec.message() << std::endl;
                                     }
                                 });

        // Run the io_service to process the asynchronous operation
        io_service->run();

    } catch (const std::exception& e) {
        throw std::runtime_error("Error sending serial data asynchronously: " + std::string(e.what()));
    }
}


void receiveSerialDataSync(const SerialPortConfig& config, std::vector<u8>& data) {
    EZ_LOG_TRACE();

    if (!io_service) {
        throw std::runtime_error("Serial IO not initialized.");
    }

    try {
        if (data.empty()) {
            throw std::invalid_argument("Data vector is empty. Provide a buffer with sufficient size.");
        }

        // Read data synchronously into the vector
        boost::asio::read(*serial_port, boost::asio::buffer(data));

        std::cout << "Synchronously received data: ";
        for (const auto& byte : data) {
            std::cout << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;

    } catch (const std::exception& e) {
        throw std::runtime_error("Error receiving serial data: " + std::string(e.what()));
    }
}


void receiveSerialDataAsync(const SerialPortConfig& config, std::vector<u8>& data) {
    EZ_LOG_TRACE();

    if (!io_service) {
        throw std::runtime_error("Serial IO not initialized.");
    }

    try {
        // Resize the data vector to the desired size

        // Create a shared pointer for the data buffer
        auto read_buffer = std::make_shared<std::vector<u8>>(data.size());

        // Asynchronously read the specified number of bytes
        boost::asio::async_read(*serial_port, boost::asio::buffer(*read_buffer),
                                [read_buffer, &data](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                                    if (!ec && bytes_transferred > 0) {
                                        // Copy the received data back to the provided vector
                                        std::copy(read_buffer->begin(), read_buffer->begin() + bytes_transferred, data.begin());
                                        std::cout << "Asynchronously received " << bytes_transferred
                                                  << " bytes of data." << std::endl;
                                    } else {
                                        std::cerr << "Error during async read: " << ec.message() << std::endl;
                                    }
                                });

        // Run the io_service to process the asynchronous operation
        io_service->run();
        serial_port->close();

    } catch (const std::exception& e) {
        throw std::runtime_error("Error receiving serial data asynchronously: " + std::string(e.what()));
    }
}


void runBenchmarkIO(const SerialPortConfig& config, const u32& data_size, const u32& iterations) {
    EZ_LOG_TRACE();
    if (!io_service) {
        throw std::runtime_error("Serial IO not initialized.");
    }

    try {

        // Allocate a buffer for sending/receiving data
        std::vector<u8> send_buffer(data_size, 0xAA); // Fill buffer with test pattern
        std::vector<u8> receive_buffer(data_size);
        const std::string default_command = "DEFAULT";
        const std::string echo_command = "ECHO";
        std::vector<u8> echo_response(default_command.size());
        boost::asio::write(*serial_port, boost::asio::buffer(default_command));

        boost::asio::write(*serial_port, boost::asio::buffer(echo_command));
        boost::asio::read(*serial_port, boost::asio::buffer(echo_response));
        // -----------------------
        // Measure Throughput
        // -----------------------
        auto start_time = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < iterations; ++i) {
            sendSerialDataSync(config, send_buffer);
            receiveSerialDataSync(config, receive_buffer);

            // Optional: Validate received data (if there's a loopback or echo test)
            if (receive_buffer != send_buffer) {
                throw std::runtime_error("Data mismatch during throughput test.");
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_time = end_time - start_time;

        double throughput = (data_size * iterations) / elapsed_time.count(); // Bytes/sec
        std::cout << "Throughput: " << throughput / 1e6 << " MB/sec" << std::endl;

        // -----------------------
        // Measure Latency
        // -----------------------
        size_t latency_iterations = 1000; // Number of latency measurements
        std::vector<u8> single_byte = {0x55};
        std::vector<u8> received_byte(1);

        start_time = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < latency_iterations; ++i) {
            sendSerialDataSync(config, single_byte);
            receiveSerialDataSync(config, received_byte);
            // Optional: Validate received byte
            if (received_byte != single_byte) {
                throw std::runtime_error("Data mismatch during latency test.");
            }
        }

        end_time = std::chrono::high_resolution_clock::now();
        elapsed_time = end_time - start_time;

        double avg_latency = (elapsed_time.count() * 1e6) / latency_iterations; // Microseconds
        std::cout << "Average Latency: " << avg_latency << " µs" << std::endl;

        serial_port->close();

    } catch (const std::exception& e) {
        throw std::runtime_error("Error during benchmarkIO: " + std::string(e.what()));
    }

}
