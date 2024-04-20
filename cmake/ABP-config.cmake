set(DEVICE_IMPL_DIR "src/Sensors/Honeywell")
# set the device header file
set(DEVICE_HEADER "ABP.hpp")

# set the device source files - these are the files that will be compiled to make this device
set(SOURCE_FILES "ABP.cpp")

add_compile_definitions(__DEVICE_CLASS=ABP)
add_compile_definitions(__DEVICE_HEADER=${DEVICE_HEADER})