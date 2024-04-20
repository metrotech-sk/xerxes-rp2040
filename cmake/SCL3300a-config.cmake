set(DEVICE_IMPL_DIR "src/Sensors/Murata")
# set the device header file
set(DEVICE_HEADER "SCL3300.hpp")

# set the device source files - these are the files that will be compiled to make this device
set(SOURCE_FILES "SCL3300a.cpp" "SCL3X00.cpp")

add_compile_definitions(__DEVICE_CLASS=SCL3300)
add_compile_definitions(__DEVICE_HEADER=${DEVICE_HEADER})
