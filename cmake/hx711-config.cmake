set(DEVICE_IMPL_DIR "src/Sensors/Generic")
# set the device header file
set(DEVICE_HEADER "hx711.hpp")
# set the device source files - these are the files that will be compiled to make this device
set(SOURCE_FILES "hx711.cpp")

add_compile_definitions(__DEVICE_CLASS=HX711)
add_compile_definitions(__DEVICE_HEADER=${DEVICE_HEADER})