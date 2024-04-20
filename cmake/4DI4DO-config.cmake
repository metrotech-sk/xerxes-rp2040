set(DEVICE_IMPL_DIR "src/Sensors/Generic/DIO")
# set the device header file
set(DEVICE_HEADER "4DI4DO.hpp")

# set the device source files - these are the files that will be compiled to make this device
set(SOURCE_FILES "4DI4DO.cpp" "DigitalInputOutput.cpp")

add_compile_definitions(__DEVICE_CLASS=_4DI4DO)
add_compile_definitions(__DEVICE_HEADER=${DEVICE_HEADER})