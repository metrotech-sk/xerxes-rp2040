# sensor-rp2040
## Overview

Sensor RP2040 is a library of sensors for the RP2040 microcontroller using shield 

## Installation

```bash
# clone this repository
git clone ###
cd sensor-rp2040

# create a debug/release build directory
mkdir build-debug  # or build-release, change debug for release
cd build-debug

cmake .. -DCMAKE_BUILD_TYPE=Debug -DSENSOR_TYPE=AI # or release, change AI for other sensor type

# put board to bootloader mode holding BOOTSEL button while pressing reset button

# build and upload firmware
make -j 16 install
```

### Sensor types

| Sensor type | Description                                                  | Manufacturer | Interface      |
|-------------|--------------------------------------------------------------|--------------|----------------|
| SCL3300     | 2 axis inclinometer, range +- 90°                            | Murata       | SPI            |
| SCL3400     | 2 axis inclinometer, range +- 30°                            | Murata       | SPI            |
| AI          | Analog input - 4 analog inputs                               | Generic      | Analog         |
| AnalogInput | Same as "AI"                                                 | Generic      | Analog         |
| 4DI4DO      | 4 digital inputs and 4 digital outputs                       | Generic      | Digital        |
| ABP         | pressure sensor, range 0-60 mbar = 0-6 kPa (differential)    | Honeywell    | I2C/SPI        |
| DEVID_TEMP_DS18B20    | Temperature sensor DS18B20                                   | Maxim         | 1-Wire         |
| DEVID_TIER            | Tier sensor                                                  | Generic      | Digital        |
| DEVID_ENC_1000PPR     | Encoder 1000 pulses per revolution                           | Generic      | Digital        |
| DEVID_IO_3AI          | 3 analog inputs                                              | Generic      | Analog         |
| DEVID_IO_4AI          | 4 analog inputs                                              | Generic      | Analog         |
| DEVID_IO_4DI_4DO      | 4 digital inputs and 4 digital outputs                       | Generic      | Digital        |
| DEVID_IO_8DI_8DO      | 8 digital inputs and 8 digital outputs                       | Generic      | Digital        |
| DEVID_LIGHT_SOUND_POLLUTION | Light and sound pollution sensor                       | Generic      | Analog/Digital |
| DEVID_PRESSURE_600MBAR | Pressure sensor, range 0-600 mbar                           | Generic      | I2C/SPI        |
| DEVID_PRESSURE_60MBAR | Pressure sensor, range 0-60 mbar                             | Generic      | I2C/SPI        |
| DEVID_STRAIN_24BIT    | Strain gauge sensor, 24-bit resolution                       | Generic      | Analog         |
| DEVID_AIR_POL_CO_NOX_VOC | Air pollution sensor for CO, NOx, and VOC                 | Generic      | I2C/SPI        |
| DEVID_AIR_POL_CO_NOX_VOC_PM | Air pollution sensor for CO, NOx, VOC, and PM          | Generic      | I2C/SPI        |
| DEVID_AIR_POL_CO_NOX_VOC_PM_GPS | Air pollution sensor for CO, NOx, VOC, PM, and GPS | Generic      | I2C/SPI        |
| DEVID_AIR_POL_PM      | Air pollution sensor for particulate matter                  | Generic      | I2C/SPI        |
| DEVID_ANGLE_XY_30     | Angle sensor, XY plane, range +- 30°                         | Generic      | Analog/Digital |
| DEVID_ANGLE_XY_90     | Angle sensor, XY plane, range +- 90°                         | Generic      | Analog/Digital |
| DEVID_CUTTER          | Cutter sensor                                                | Generic      | Digital        |
| DEVID_DIST_225MM      | Distance sensor, range 225 mm                                | Generic      | Analog/Digital |
| DEVID_DIST_22MM       | Distance sensor, range 22 mm                                 | Generic      | Analog/Digital |
| DEVID_ACCEL_XYZ       | Accelerometer, 3-axis                                        | Generic      | I2C/SPI        |


## Other remarks
### low latency USB Serial
```shell
echo 1 | sudo tee /sys/bus/usb-serial/devices/ttyUSB0/latency_timer  # change ttyUSB0 for your device
```

### Using Udev Rules under /etc/udev/rules.d/50-custom.rules:
```shell
KERNEL=="tty[A-Z]*[0-9]|pppox[0-9]*|ircomm[0-9]*|noz[0-9]*|rfcomm[0-9]*", GROUP="dialout"

# USB latency rules
ACTION=="add", SUBSYSTEM=="usb-serial", DRIVER=="ftdi_sio", ATTR{latency_timer}="1"
```

## Changelog
### 2024.12.17 - SR 
* Removed LDO for 3V3_EXT
* removed USR-switch
* replaced MAX13487 with SP3485EN-L/TR (10Mbps)
