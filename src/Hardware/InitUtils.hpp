#ifndef __INITUTILS_H
#define __INITUTILS_H

#include <stdint.h>

/**
 * @brief Initialize the queues for the UART
 */
void userInitQueue();

/**
 * @brief Interrupt handler for the UART
 *
 * This function is called when the UART receives a byte. The byte is received and then pushed to txFifo
 */
void uart_interrupt_handler();

/**
 * @brief Initialize the UART
 *
 * This function initializes the UART and sets the interrupt handler `uart_interrupt_handler`. A RS485 transceiver is also initialized
 */
void userInitUart(void);

/**
 * @brief Initialize the GPIO pins
 *
 * This function initializes the GPIO pins for the user LED, Button and Switch
 */
void userInitGpio();

/**
 * @brief Clear the flash and load default values a.k.a. FACTORY RESET
 */
void userLoadDefaultValues();

/**
 * @brief Initialize the micro-controller using the Pico SDK.
 *
 * This function initializes the clocks, GPIO pins, UART, flash memory and queues, if necessary it also loads the default values
 *
 */
void userInit();

#endif // !__INITUTILS_H