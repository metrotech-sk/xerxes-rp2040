set(DEVICE_IMPL_DIR "src/Sensors/Generic/ds18b20")
# set the device header file
set(DEVICE_HEADER "ds18b20.hpp")

# set the device source files - these are the files that will be compiled to make this device
set(SOURCE_FILES "ds18b20.cpp")

add_compile_definitions(__DEVICE_CLASS=DS18B20)
add_compile_definitions(__DEVICE_HEADER=${DEVICE_HEADER})