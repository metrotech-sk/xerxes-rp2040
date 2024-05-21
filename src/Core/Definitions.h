#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include "hardware/flash.h"
#include "hardware/clocks.h"
#include "MemoryMap.h"

/** @brief Default baudrate for serial communication */
#define DEFAULT_BAUDRATE 57600

#define VOLATILE_OFFSET FLASH_PAGE_SIZE      // 256 bytes
#define READ_ONLY_OFFSET FLASH_PAGE_SIZE * 2 // 512 bytes
#define MESSAGE_OFFSET FLASH_PAGE_SIZE * 3   // 768 bytes
#define REGISTER_SIZE FLASH_PAGE_SIZE * 4    // 1024 bytes

#define RX_TX_QUEUE_SIZE 256 ///< 256 bytes
#define FIFO_DEPTH 32        ///< 32 bytes

/// @brief Use last sector of flash for storing data
#define FLASH_TARGET_OFFSET PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE

// how many samples are rotated in ring buffer
#ifndef RING_BUFFER_LEN
#define RING_BUFFER_LEN 100
#endif // !RING_BUFFER_LEN

/* config masks */
/* If true use free run, if false: wait for sync packet */
#define MASK_CONFIG_FREE_RUN 1 << 0
/* if true, enable automatic calculation of the statistics */
#define MASK_CONFIG_CALC_STATS 1 << 1

/* Default values */
#ifndef DEFAULT_CYCLE_TIME_US
#define DEFAULT_CYCLE_TIME_US 10000 // 10 ms
#endif                              // !DEFAULT_CYCLE_TIME_US

#ifndef DEFAULT_WATCHDOG_DELAY
#define DEFAULT_WATCHDOG_DELAY 200 // ms
#endif                             // !DEFAULT_WATCHDOG_DELAY

/* Default clocks */
#define DEFAULT_USB_CLOCK_FREQ 48 * MHZ
#define DEFAULT_XOSC_CLOCK_FREQ 12 * MHZ

#define DEFAULT_ADC_CLOCK_SRC CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB // maybe use XOSC
#define DEFAULT_ADC_CLOCK_FREQ DEFAULT_USB_CLOCK_FREQ
#define DEFAULT_PERI_CLOCK_SRC CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB
#define DEFAULT_PERI_CLOCK_FREQ DEFAULT_USB_CLOCK_FREQ

/* system clock */
#define DEFAULT_SYS_CLOCK_SRC CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS
#define DEFAULT_SYS_CLOCK_SRC_LP CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF
#define DEFAULT_SYS_PLL_FREQ 756 * MHZ
#define DEFAULT_SYS_PLL_POST_1 6
#define DEFAULT_SYS_PLL_POST_2 1
#define DEFAULT_SYS_CLOCK_FREQ DEFAULT_SYS_PLL_FREQ / DEFAULT_SYS_PLL_POST_1 / DEFAULT_SYS_PLL_POST_2 // 756MHz / 6 / 1 = 126MHz
#define DEFAULT_SYS_CLOCK_FREQ_LP DEFAULT_XOSC_CLOCK_FREQ
#define DEFAULT_SYS_VOLTAGE VREG_VOLTAGE_DEFAULT // 1.1V
#define DEFAULT_SYS_VOLTAGE_LP VREG_VOLTAGE_1_00 // 1V (for low power mode), less is not stable

#endif // DEFINITIONS_H