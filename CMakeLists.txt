cmake_minimum_required(VERSION 3.22)

# this must be first to set the correct toolchain
include(pico_sdk_import.cmake)
# We also need PICO EXTRAS
include(pico_extras_import.cmake)

set(PROJECT_NAME sensor-rp2040)
project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 11)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(DEVICE_TYPES "SCL3300" "SCL3300a" "SCL3400" "AnalogInput" "DiscreteAnalog" 
"4DI4DO" "ABP" "hx711" "Encoder" "Cutter" "LightSound" "temperature" "lis2dw12" "LIS2") 
set(DEVICE_TYPE_HINT "Possible options for DEVICE_TYPE: ${DEVICE_TYPES}")

# this is the board we are using, we need to set this to use the correct board header and bootloader
# the board file is: src/Hardware/Board/xerxes_rp2040.h
set(PICO_BOARD "xerxes_rp2040")
set(PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_LIST_DIR}/src/Hardware/Board")

# parse version and build number from git
execute_process(
    COMMAND git describe --long
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_compile_definitions(__VERSION="${VERSION}")
message("Version: ${VERSION}")

if(NOT DEFINED DEVICE_ADDRESS)
    set(DEVICE_ADDRESS 0)
endif()

message("DEVICE_ADDRESS set to ${DEVICE_ADDRESS}")
add_compile_definitions(__DEVICE_ADDRESS=${DEVICE_ADDRESS})

# check if ${CLKDIV} is set and set it to 4 if not - necessary for some boards
if(NOT DEFINED CLKDIV)
	set(CLKDIV 4)
endif()
add_compile_definitions(__CLKDIV=${CLKDIV})

if(NOT DEFINED LOG_LEVEL)
	set(LOG_LEVEL 1)
endif()
add_compile_definitions(_LOG_LEVEL=${LOG_LEVEL})
if(${LOG_LEVEL} EQUAL 1)
    message("LOG_LEVEL set to ${LOG_LEVEL} = ERROR")
elseif(${LOG_LEVEL} EQUAL 2)
    message("LOG_LEVEL set to ${LOG_LEVEL} = WARNING")
elseif(${LOG_LEVEL} EQUAL 3)
    message("LOG_LEVEL set to ${LOG_LEVEL} = INFO")
elseif(${LOG_LEVEL} EQUAL 4)
    message("LOG_LEVEL set to ${LOG_LEVEL} = DEBUG")
elseif(${LOG_LEVEL} EQUAL 5)
    message("LOG_LEVEL set to ${LOG_LEVEL} = TRACE")
endif()

# check if ${DEVICE_TYPE} is set and is one of the possible options
if(NOT DEFINED DEVICE_TYPE) 
	message(FATAL_ERROR "DEVICE_TYPE not set, use -DDEVICE_TYPE=...\n${DEVICE_TYPE_HINT}")
elseif(NOT ${DEVICE_TYPE} IN_LIST DEVICE_TYPES)
	message(FATAL_ERROR "DEVICE_TYPE '${DEVICE_TYPE}' is incorrect, use -DDEVICE_TYPE=...\n${DEVICE_TYPE_HINT}")
endif()


# include the correct device config file based on the DEVICE_TYPE set
include(device-config)  # this will set DEVICE_SOURCES and DEVICE_HEADER
if(NOT DEFINED DEVICE_HEADER)
	message(FATAL_ERROR "DEVICE_HEADER not set")
endif()
message("DEVICE_HEADER set to ${DEVICE_HEADER}")

# Create the header file using configure_file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sensors/SensorHeader.hpp.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sensors/SensorHeader.hpp
)

# print warning to clearly show which device type is used and which CLKDIV is set
message(WARNING "\nDEVICE_TYPE set to '${DEVICE_TYPE}'."
	"\nCLKDIV set to ${CLKDIV}, increase this "
	"value if device is not responding after reboot.")

pico_sdk_init()
 
# find xerxes-protocol library
set(xerxes-protocol_DIR "lib/xerxes-protocol-cpp")
find_package(xerxes-protocol REQUIRED)
add_library(xerxes-protocol STATIC ${xerxes-protocol_SOURCES})

include_directories(
	"src"
	${xerxes-protocol_INCLUDE_DIRS}
)

add_executable(
	${PROJECT_NAME}
	src/Communication/Callbacks.cpp
	src/Hardware/ClockUtils.cpp
	src/Hardware/Sleep.cpp
	src/Hardware/InitUtils.cpp
	src/Hardware/UserFlash.cpp
	src/Communication/RS485.cpp
	src/Core/Slave.cpp
	src/Core/Register.cpp
	src/Sensors/Peripheral.cpp
	src/Sensors/Sensor.cpp
	src/Utils/Gpio.cpp

	${DEVICE_SOURCES}  # this is automatically generated in cmake/device-config.cmake

	src/main.cpp
)

target_link_libraries(
	${PROJECT_NAME} 
	pico_stdlib 
	pico_multicore
	hardware_rtc
	hardware_gpio
	hardware_adc
	hardware_uart
	hardware_pwm
	hardware_sleep
	hardware_flash
	hardware_sync
	hardware_spi
    hardware_i2c
	xerxes-protocol
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${DEVICE_TYPE})

# enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
# pico_enable_stdio_uart(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})

# copy .UF2 binary to the RP2040
install(
	FILES
	${CMAKE_CURRENT_BINARY_DIR}/${DEVICE_TYPE}.uf2
	DESTINATION /media/$ENV{USER}/RPI-RP2/${DEVICE_TYPE}.uf2
)