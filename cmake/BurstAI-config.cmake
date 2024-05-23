set(DEVICE_IMPL_DIR "src/Sensors/Generic")

# set the device header file
set(DEVICE_HEADER "AnalogInput.hpp")

# set the device source files - these are the files that will be compiled to make this device
set(SOURCE_FILES "AnalogInput.cpp")

add_compile_definitions(__DEVICE_CLASS=AnalogInput)
add_compile_definitions(__DEVICE_HEADER=${DEVICE_HEADER})
add_compile_definitions(__TIGHTLOOP)