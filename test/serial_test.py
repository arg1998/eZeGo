import serial
import time

# Set up the serial connection (replace with your port and baudrate)
ser = serial.Serial('/dev/tty.usbmodem11303', 115200, timeout=2)  # Change 'COMx' to your device's port
time.sleep(2)  # Wait for the serial connection to establish

def send_command(command):
    """Send a command to the UART and read the response."""
    # Send the command
    ser.write(command.encode())  # Convert string to bytes and send
    print(f"Sent: {command}")

    # Wait a moment for the response
    time.sleep(0.2)  # Adjust based on your response time

    # Read the response
    response = ser.read(ser.in_waiting)  # Read all available data
    return response.decode()  # Convert bytes to string

def main():
    # Test the system with the "ECHO" command
    response = send_command("ECHO")
    print(f"Received: {response}")

    # Send the "DEFAULT" command
    response = send_command("DEFAULT")
    print(f"Received: {response}")

    # You can add more tests or loop through commands
    # For example, send random data and check response
    while True:
        command = input("Enter command (ECHO/DEFAULT): ")
        response = send_command(command)
        print(f"Received: {response}")

if __name__ == "__main__":
    main()
