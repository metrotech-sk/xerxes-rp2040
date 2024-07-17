#ifndef __ERROR_H
#define __ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    enum class ERROR_NUM
    {
        UART_OVERLOAD = 0,
        CPU_OVERLOAD = 1,
        BUS_COLLISION = 2,
        WATCHDOG_TIMEOUT = 3,
        SENSOR_OVERLOAD = 4,
        SENSOR_CONNECTION = 5,
        SENSOR_CONNECTION_MEM = 6,
        DEVICE_INIT = 7,
        SENSOR_FAULT = 8
    };

    /**
     * @brief Error codes for the error register
     *
     */
    enum : uint64_t
    {
        ERROR_MASK_UART_OVERLOAD = 1 << static_cast<uint64_t>(ERROR_NUM::UART_OVERLOAD),                 // 0b1 = 1
        ERROR_MASK_CPU_OVERLOAD = 1 << static_cast<uint64_t>(ERROR_NUM::CPU_OVERLOAD),                   // 0b10 = 2
        ERROR_MASK_BUS_COLLISION = 1 << static_cast<uint64_t>(ERROR_NUM::BUS_COLLISION),                 // 0b100 = 4
        ERROR_MASK_WATCHDOG_TIMEOUT = 1 << static_cast<uint64_t>(ERROR_NUM::WATCHDOG_TIMEOUT),           // 0b1000 = 8
        ERROR_MASK_SENSOR_OVERLOAD = 1 << static_cast<uint64_t>(ERROR_NUM::SENSOR_OVERLOAD),             // 0b10000 = 16
        ERROR_MASK_SENSOR_CONNECTION = 1 << static_cast<uint64_t>(ERROR_NUM::SENSOR_CONNECTION),         // 0b100000 = 32
        ERROR_MASK_SENSOR_CONNECTION_MEM = 1 << static_cast<uint64_t>(ERROR_NUM::SENSOR_CONNECTION_MEM), // 0b1000000 = 64
        ERROR_MASK_DEVICE_INIT = 1 << static_cast<uint64_t>(ERROR_NUM::DEVICE_INIT),                     // 0b10000000 = 128
        ERROR_MASK_SENSOR_FAULT = 1 << static_cast<uint64_t>(ERROR_NUM::SENSOR_FAULT)                    // 0b100000000 = 256
    };

#ifdef __cplusplus
}
#endif

#endif // __ERROR_H