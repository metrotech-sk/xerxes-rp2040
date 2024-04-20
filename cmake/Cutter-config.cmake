set(DEVICE_IMPL_DIR "src/Sensors/Generic/DIO")
# set the device header file
set(DEVICE_HEADER "Cutter.hpp")

# set the device source files - these are the files that will be compiled to make this device
set(SOURCE_FILES "4DI4DO.cpp" "DigitalInputOutput.cpp" "Encoder.cpp" "Cutter.cpp")

add_compile_definitions(__DEVICE_CLASS=Cutter)
add_compile_definitions(__DEVICE_HEADER=${DEVICE_HEADER})
add_compile_definitions(__SHIELD_CUTTER)
add_compile_definitions(__TIGHTLOOP)