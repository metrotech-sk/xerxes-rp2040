#include <bitset>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "pico/util/queue.h"

#include "Core/Errors.h"
#include "Core/BindWrapper.hpp"
#include "Core/Slave.hpp"
#include "Core/Register.hpp"
#include "Communication/Callbacks.hpp"
#include "Hardware/Board/xerxes_rp2040.h"
#include "Hardware/ClockUtils.hpp"
#include "Hardware/InitUtils.hpp"
#include "Hardware/Sleep.hpp"
#include "Sensors/SensorHeader.hpp"
#include "Communication/RS485.hpp"
#include "Utils/Log.h"
#include "Utils/Gpio.hpp"

using namespace std;
using namespace Xerxes;

// forward declaration
__DEVICE_CLASS device;

Register _reg; // main register

/// @brief transmit FIFO queue for UART
queue_t txFifo;
/// @brief receive FIFO queue for UART
queue_t rxFifo;

volatile bool usrSwitchOn;      // user switch state
volatile bool core1idle = true; // core1 idle flag
volatile bool useUsb = false;   // use usb uart flag
volatile bool awake = true;

/**
 * @brief Core 1 entry point, runs in background
 */
void core1Entry();

enum class CTRL
{
    SOH = 0x01,
    STX = 0x02,
    ETX = 0x03,
    EOT = 0x04
};

bool repeating_timer_callback(struct repeating_timer *t)
{
    xlog_dbg("Repeat timer callback");
    return true;
}

void send_empty_sync_packet()
{
    // send empty sync packet SOH + STX + ETX + EOT

    uint8_t syncPacket[4] = {CTRL::SOH, CTRL::STX, CTRL::ETX, CTRL::EOT};
    uart_write_blocking(uart0, syncPacket, 4);
}

bool expect(char expected)
{
    uint8_t data;
    uart_read_blocking(uart0, &data, 1);
    if (data == expected)
    {
        return true;
    }
    else
    {
        xlog_trace("Expected %d, got %d", expected, data);
        return false;
    }
}

void wait_for_sequence(const uint8_t *sequence, size_t length)
{
    size_t index = 0;

    while (true)
    {
        if (expect(sequence[index]))
        {
            index++;
            if (index == length)
            {
                return; // Entire sequence matched, exit the function
            }
        }
        else
        {
            index = 0; // Reset if the expected character is not received
        }
    }
}

void wait_for_sync()
{
    uint8_t *syncPacket = (const uint8_t[]){CTRL::SOH, CTRL::STX, CTRL::ETX, CTRL::EOT};
    wait_for_sequence(syncPacket, sizeof(syncPacket);
    xlog_dbg("Synced");
}

int main(void)
{
    // enable watchdog for 200ms, pause on debug = true
    watchdog_enable(DEFAULT_WATCHDOG_DELAY, true);

    // init system
    userInit(); // 374us

    // blink led for 10 ms - we are alive
    gpio_put(USR_LED_PIN, 1);
    sleep_ms(10);
    gpio_put(USR_LED_PIN, 0);

    // clear error register
    _reg.errorClear(0xFFFFFFFF);

    // determine reason for restart:
    if (watchdog_caused_reboot())
    {
        _reg.errorSet(ERROR_MASK_WATCHDOG_TIMEOUT);
    }

    // check if user switch is on, if so, use usb uart
    useUsb = gpio_get(USR_SW_PIN);

    // if user button is pressed, load default values a.k.a. FACTORY RESET
    if (!gpio_get(USR_BTN_PIN))
        userLoadDefaultValues();

    if (useUsb)
    {
        // init usb uart
        userInitUartDisabled();

        while (!stdio_usb_connected()) // wait for usb connection
        {
            watchdog_update();
        }
        xlog_info("USB Connected");
    }
    else
    {
        stdio_usb_init();
    }

    watchdog_update();
    device = __DEVICE_CLASS(&_reg);
    watchdog_update();

    if (useUsb)
    {

        cout << device.getInfoJson() << endl;
    }

    // init uart over RS485
    userInitUart();

    // Create a repeating timer that calls repeating_timer_callback.
    // If the delay is > 0 then this is the delay between the previous callback ending and the next starting.
    // If the delay is negative (see below) then the next call to the callback will be exactly 500ms after the
    // start of the call to the last callback
    struct repeating_timer timer;
    // add_repeating_timer_ms(500, repeating_timer_callback, NULL, &timer);

    add_repeating_timer_us(100000, repeating_timer_callback, NULL, &timer);

    // main loop, runs forever, handles all communication in this loop
    while (1)
    {
        wait_for_sync();
    }
    while (0)
    {
        // update watchdog
        watchdog_update();

        // running on RS485, sync for incoming messages from master, timeout
        // = 5ms
        xlog_trace("Syncing xerxes network");
        xs.sync(5000);

        // send char if tx queue is not empty and uart is writable
        if (!queue_is_empty(&txFifo))
        {
            ledComOn(); // set communication activity led
            xlog_trace("got some data to process");
            uint txLen = queue_get_level(&txFifo);
            assert(txLen <= RX_TX_QUEUE_SIZE);

            uint8_t toSend[txLen];

            // drain queue
            for (uint i = 0; i < txLen; i++)
            {
                queue_remove_blocking(&txFifo, &toSend[i]);
            }
            xlog_trace("Writing data to uart");
            // write char to bus, this will clear the interrupt
            uart_write_blocking(uart0, toSend, txLen);
            ledComOff(); // clear communication activity led
        }

        if (queue_is_full(&txFifo) || queue_is_full(&rxFifo))
        {
            // rx fifo is full, set the cpu_overload error flag
            xlog_error("Queue is full");
            _reg.errorSet(ERROR_MASK_UART_OVERLOAD);
        }
    }
}

void core1Entry()
{
    xlog_dbg("Entering core 1");

    // let core0 lockout core1
    multicore_lockout_victim_init();

    // not used yet
}