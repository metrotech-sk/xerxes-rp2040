set(DEVICE_IMPL_DIR "src/Sensors/ST")
# set the device header file
set(DEVICE_HEADER "Lis2dw12.hpp")

# set the device source files - these are the files that will be compiled to make this device
set(SOURCE_FILES "Lis2dw12.cpp")

add_compile_definitions(__DEVICE_CLASS=LIS2)
add_compile_definitions(__DEVICE_HEADER=${DEVICE_HEADER})
add_compile_definitions(__TIGHTLOOP)