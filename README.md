# Xerxes RP2040 Sensor Platform

A comprehensive sensor firmware for RP2040 microcontroller supporting 20+ sensor types with RS485 communication using the Xerxes protocol.

## Overview

The Xerxes RP2040 platform provides a unified firmware framework for industrial sensor applications. It features:

- **Multi-sensor support**: 20+ sensor types from various manufacturers (Murata, Honeywell, ST Microelectronics)
- **Dual-core architecture**: Core 0 handles communication, Core 1 processes sensor data
- **RS485/UART communication**: Industrial-grade networking with Xerxes protocol
- **Python utilities**: Device discovery, calibration, and data acquisition tools
- **Real-time processing**: Configurable sampling rates with statistical analysis

## Quick Start

### Prerequisites
- **Hardware**: RP2040-based Xerxes sensor board
- **Software**: CMake, ARM GCC toolchain, Pico SDK
- **Python**: Python 3.8+ with xerxes-protocol package

### Build and Flash

```bash
# Clone repository with submodules
git clone --recursive https://github.com/metrotech-sk/xerxes-rp2040.git
cd xerxes-rp2040

# Create build directory
mkdir build && cd build

# Configure for your sensor type (example: analog input)
cmake .. -DCMAKE_BUILD_TYPE=Release -DDEVICE_TYPE=AnalogInput -DDEVICE_ADDRESS=1

# Build firmware
make -j$(nproc)

# Flash: Put board in bootloader mode (hold BOOTSEL + reset)
make install
```

### Python Setup

```bash
# Install Python dependencies
pip install -r requirements.txt

# Discover devices on network
python utils/discover.py --port /dev/ttyUSB0

# Read sensor values in real-time
python utils/read_pvs.py --address 1 --port /dev/ttyUSB0
```

## Supported Sensors

### Digital Inclinometers
| Type | Range | Resolution | Interface | Manufacturer |
|------|-------|------------|-----------|--------------|
| SCL3300 | ±90° | 0.002° | SPI | Murata |
| SCL3400 | ±30° | 0.001° | SPI | Murata |

### Analog/Digital I/O
| Type | Description | Channels | Interface |
|------|-------------|----------|-----------|
| AnalogInput | 12-bit ADC with oversampling | 4 inputs | ADC |
| 4DI4DO | Digital I/O module | 4 in + 4 out | GPIO |

### Pressure Sensors
| Type | Range | Interface | Manufacturer |
|------|-------|-----------|--------------|
| ABP | 0-60 mbar differential | I2C/SPI | Honeywell |

### Environmental Sensors
| Type | Measurement | Interface | Notes |
|------|-------------|-----------|-------|
| DS18B20 | Temperature | 1-Wire | Multiple sensors on bus |
| LIS2DW12 | 3-axis acceleration | I2C/SPI | ST Microelectronics |

## Usage Examples

### Basic Sensor Reading

```python
from xerxes_protocol import XerxesNetwork, XerxesRoot
from serial import Serial

# Connect to network
serial = Serial('/dev/ttyUSB0', 115200)
network = XerxesNetwork(serial).init()
root = XerxesRoot(network)

# Read from device at address 1
device = root.getLeaf(1)
values = device.readProcessValues()
print(f"Sensor values: {values}")
```

### Device Configuration

```python
# Set sampling rate to 100Hz
device.writeRegister('desiredCycleTimeUs', 10000)

# Enable free-running mode
device.writeRegister('config', {'freeRun': True, 'calcStat': True})

# Calibrate sensor (zero offset)
device.calibrate()
```

### Batch Data Collection

```bash
# Continuous logging to file
python utils/read_pvs.py --address 1 --output data.csv --duration 60

# Calibrate multiple devices
python utils/calibrate.py --address-range 1-10
```

## Architecture

### Firmware Structure
```
src/
├── Core/           # Main application logic, registers, communication
├── Sensors/        # Device drivers (Generic, Murata, Honeywell, ST)
├── Communication/  # RS485/UART interface and protocol handlers
├── Hardware/       # Board definitions, initialization, power management
└── Utils/          # Logging, GPIO, FFT processing
```

### Communication Protocol
- **Physical layer**: RS485 with automatic direction control
- **Protocol**: Xerxes binary protocol with CRC
- **Addressing**: 8-bit device addresses (1-254)
- **Commands**: Read/write registers, ping, sync, reset, sleep

### Build Configuration
```bash
# Available device types
DEVICE_TYPE: AnalogInput, SCL3300, SCL3400, 4DI4DO, ABP, temperature, etc.

# Optional parameters
-DDEVICE_ADDRESS=1      # Device network address (0-254)
-DLOG_LEVEL=3           # 1=Error, 2=Warning, 3=Info, 4=Debug, 5=Trace
-DCLKDIV=8              # Clock divider for timing-sensitive sensors
```

## Development

### Adding New Sensors

1. Create sensor driver in `src/Sensors/{Manufacturer}/`
2. Implement required interface methods
3. Add device type to `cmake/device-config.cmake`
4. Test with provided utilities

### Testing

```bash
# Run unit tests
cd tests && python -m pytest

# Hardware-in-loop testing
python tests/read_distance_means.py --device-type AnalogInput
```

## Troubleshooting

### Common Issues

**Device not responding:**
- Check wiring and power supply
- Verify correct DEVICE_ADDRESS
- Increase CLKDIV if timing issues occur

**Communication errors:**
- Ensure proper RS485 termination
- Check baud rate settings (default: 115200)
- Verify USB latency settings for development

**Build failures:**
- Update git submodules: `git submodule update --init --recursive`
- Install Pico SDK and set PICO_SDK_PATH

### USB Serial Optimization

```bash
# Reduce latency for development (Linux)
echo 1 | sudo tee /sys/bus/usb-serial/devices/ttyUSB0/latency_timer

# Udev rules (/etc/udev/rules.d/50-xerxes.rules)
KERNEL=="ttyUSB*", ATTRS{idVendor}=="0403", ATTR{latency_timer}="1"
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Changelog

### 2024.12.17 - SR 
- Removed LDO for 3V3_EXT
- Removed USR-switch  
- Replaced MAX13487 with SP3485EN-L/TR (10Mbps)
