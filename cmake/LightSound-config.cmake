set(DEVICE_IMPL_DIR "src/Sensors/Generic/Enviro")
# set the device header file
set(DEVICE_HEADER "LightSound.hpp")

# set the device source files - these are the files that will be compiled to make this device
set(SOURCE_FILES "LightSound.cpp")

add_compile_definitions(__DEVICE_CLASS=LightSound)
add_compile_definitions(__DEVICE_HEADER=${DEVICE_HEADER})