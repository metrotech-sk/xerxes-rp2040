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

#ifndef __SAMPLING_FREQUENCY
#define __SAMPLING_FREQUENCY 100 // Hz
#endif

const int32_t _TIMER_DELAY_US = -1000000 / __SAMPLING_FREQUENCY; // 100Hz

/**
 * @brief Core 1 entry point, runs in background
 */
void core1Entry();
bool repeating_timer_callback(struct repeating_timer *t);
bool expect(const char expected);
void wait_for_sync();
void sync();
void make_packet(std::vector<uint8_t> &packet);
void dump_packet(const std::vector<uint8_t> &packet);

namespace CTRL
{
    const uint8_t SOH = 0x01;
    const uint8_t STX = 0x02;
    const uint8_t ETX = 0x03;
    const uint8_t EOT = 0x04;
};

void make_packet(std::vector<uint8_t> &packet)
{
    // dump data to vector
    packet.push_back(CTRL::SOH);
    packet.push_back(CTRL::STX);
    uint8_t bytes[sizeof(float)];
    std::memcpy(bytes, _reg.pv0, sizeof(float));
    packet.insert(packet.end(), bytes, bytes + sizeof(float));
    std::memcpy(bytes, _reg.pv1, sizeof(float));
    packet.insert(packet.end(), bytes, bytes + sizeof(float));
    std::memcpy(bytes, _reg.pv2, sizeof(float));
    packet.insert(packet.end(), bytes, bytes + sizeof(float));
    std::memcpy(bytes, _reg.pv3, sizeof(float));
    packet.insert(packet.end(), bytes, bytes + sizeof(float));
    packet.push_back(CTRL::ETX);
    packet.push_back(CTRL::EOT);
}

void dump_packet(const std::vector<uint8_t> &packet)
{
    // dump the data to the bus
    uart_write_blocking(uart0, packet.data(), packet.size());
}

bool repeating_timer_callback(struct repeating_timer *t)
{
    ledBusyOn();
    device.update();
    std::vector<uint8_t> packet;
    make_packet(packet);
    ledBusyOff();

    sync();
    dump_packet(packet);

    /*
    TODO: send data too
    */
    return true;
}

bool expect(const char expected)
{
    uint8_t data;
    // uart_read_blocking(uart0, &data, 1); // we using queue
    queue_remove_blocking(&rxFifo, &data);

    if (data == expected)
    {
        return true;
    }
    else
    {
        xlog_trace("Expected " << expected << " but got " << data);
        return false;
    }
}

void wait_for_sync()
{
    while (1)
    {
        if (expect(CTRL::SOH))
        {
            if (expect(CTRL::STX))
            {
                for (size_t i = 0; i < 8; i++)
                {
                    uint8_t data;
                    queue_remove_blocking(&rxFifo, &data);
                }
                if (expect(CTRL::ETX))
                {
                    if (expect(CTRL::EOT))
                    {
                        break;
                    }
                }
            }
        }
    }
}

void sync()
{
    uint64_t timestamp = time_us_64();
    // construct sync packet out of SOH STX <timestamp> ETX EOT
    std::vector<uint8_t> syncPacket = {CTRL::SOH, CTRL::STX};
    {
        uint8_t bytes[sizeof(timestamp)];
        std::memcpy(bytes, &timestamp, sizeof(uint64_t));
        syncPacket.insert(syncPacket.end(), bytes, bytes + sizeof(uint64_t));
    }
    syncPacket.push_back(CTRL::ETX);
    syncPacket.push_back(CTRL::EOT);
    // send sync packet
    ledComOn();
    uart_write_blocking(uart0, syncPacket.data(), syncPacket.size());
}

int main(void)
{
    // enable watchdog for 200ms, pause on debug = true
    // watchdog_enable(DEFAULT_WATCHDOG_DELAY, true);

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
    device.init();

    if (useUsb)
    {

        cout << device.getInfoJson() << endl;
    }

    // init uart over RS485
    userInitUart();

    // start core1 for device operation
    // multicore_launch_core1(core1Entry); // to spin on watchdog

    // Create a repeating timer that calls repeating_timer_callback.
    // If the delay is > 0 then this is the delay between the previous callback ending and the next starting.
    // If the delay is negative (see below) then the next call to the callback will be exactly XYus after the
    // start of the call to the last callback
    struct repeating_timer timer;

    // main loop, runs forever, handles all communication in this loop
    if (__DEVICE_ADDRESS == 0)
    {
        add_repeating_timer_us(_TIMER_DELAY_US, repeating_timer_callback, NULL, &timer); // sync every 10ms
        while (1)
        {
            busy_wait_us(10);
        }
    }
    else
    {
        while (1)
        {
            ledBusyOn();
            device.update();
            ledBusyOff();
            std::vector<uint8_t> packet;
            make_packet(packet);

            wait_for_sync();
            absolute_time_t window = make_timeout_time_us(__DEVICE_ADDRESS * 1000);
            // wait until the time window
            while (!time_reached(window))
            {
                busy_wait_us(1);
            }
            dump_packet(packet);
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