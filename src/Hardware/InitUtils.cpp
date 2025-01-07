#include "InitUtils.hpp"

#include "Board/xerxes_rp2040.h"
#include "ClockUtils.hpp"
#include "Core/Errors.h"
#include "UserFlash.hpp"
#include "Core/Definitions.h"
#include "Core/Register.hpp"

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/flash.h"
#include "hardware/rtc.h"
#include "pico/util/queue.h"

extern Xerxes::Register _reg;
extern queue_t txFifo, rxFifo;

void userInitQueue()
{
    queue_init(&txFifo, 1, RX_TX_QUEUE_SIZE);
    queue_init(&rxFifo, 1, RX_TX_QUEUE_SIZE);
}

void uart_interrupt_handler()
{
    gpio_put(LED_COM_ACT_PIN, 1);

    if (uart_is_readable(uart0))
    {
        unsigned char rcvd = uart_getc(uart0);
        auto success = queue_try_add(&rxFifo, &rcvd);

        if (!success)
        {
            // set cpu overload flag
            *_reg.error |= ERROR_MASK_CPU_OVERLOAD;
        }
    }

    gpio_put(LED_COM_ACT_PIN, 0);
    irq_clear(UART0_IRQ);
}

void userInitUart()
{
    // Initialise UART 0 on 115200baud
    uart_init(uart0, DEFAULT_BAUDRATE);

    // Set the GPIO pin mux to the UART - 16 is TX, 17 is RX
    gpio_set_function(RS_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RS_RX_PIN, GPIO_FUNC_UART);

    // initialise RS485 enable pin and set it to high, this will enable the transceiver
    gpio_init(RS_EN_PIN);
    gpio_set_dir(RS_EN_PIN, GPIO_OUT);
    gpio_put(RS_EN_PIN, false); // set to low to enable receive

    // enable fifo for uart, each FIFO is 32 levels deep
    uart_set_fifo_enabled(uart0, true);

    // disable stdio uart
    // stdio_set_driver_enabled(&stdio_uart, false);

    // set uart interrupt handler, must be done before enabling uart interrupt
    irq_set_exclusive_handler(UART0_IRQ, uart_interrupt_handler);
    // enable uart interrupt
    irq_set_enabled(UART0_IRQ, true);

    // enable uart interrupt for receiving and transmitting
    uart_set_irq_enables(uart0, true, false);
}

void userInitGpio()
{
    // initialize the user led and button pins
    gpio_init(LED_COM_ACT_PIN);
    gpio_init(USR_LED_PIN);
    gpio_init(USR_BTN_PIN);

    gpio_set_drive_strength(USR_LED_PIN, GPIO_DRIVE_STRENGTH_2MA);

    gpio_set_dir(USR_LED_PIN, GPIO_OUT);
    gpio_set_dir(LED_COM_ACT_PIN, GPIO_OUT);
    gpio_set_dir(USR_BTN_PIN, GPIO_IN);

    gpio_pull_up(USR_BTN_PIN);
}

void userLoadDefaultValues()
{
    // set default values for the registers
    for (uint i = 0; i < REGISTER_SIZE; i++)
    {
        _reg.memTable[i] = 0;
    }

    // set default values for the gains, unit gains
    *_reg.gainPv0 = 1;
    *_reg.gainPv1 = 1;
    *_reg.gainPv2 = 1;
    *_reg.gainPv3 = 1;

    *_reg.desiredCycleTimeUs = DEFAULT_CYCLE_TIME_US;
    _reg.config->bits.calcStat = 1;
    _reg.config->bits.freeRun = 1;
    *_reg.devAddress = __DEVICE_ADDRESS;
    updateFlash((uint8_t *)_reg.memTable);
}

void userInit()
{
    // initialize the clocks
    userInitClocks();

    // initialize the gpios
    userInitGpio();

    // initialize the queues for uart communication
    userInitQueue();

    // initialize the flash memory and load the default values
    if (!userInitFlash((uint8_t *)_reg.memTable))
    {
        userLoadDefaultValues();
    }
}
