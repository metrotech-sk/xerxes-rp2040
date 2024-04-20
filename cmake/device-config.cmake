include("${DEVICE_TYPE}-config")

include_directories("${DEVICE_IMPL_DIR}")
foreach(SOURCE_FILE ${SOURCE_FILES})
    list(APPEND DEVICE_SOURCES "${DEVICE_IMPL_DIR}/${SOURCE_FILE}")
endforeach()
message("Device sources: ${DEVICE_SOURCES}")