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
#include "Utils/Various.hpp"

using namespace std;
using namespace Xerxes;

// forward declaration
__DEVICE_CLASS device;

Register _reg; // main register

/// @brief transmit FIFO queue for UART
queue_t txFifo;
/// @brief receive FIFO queue for UART
queue_t rxFifo;

RS485 xn(&txFifo, &rxFifo); // RS485 interface
Protocol xp(&xn);           // Xerxes protocol implementation
Slave xs;

volatile bool usrSwitchOn;      // user switch state
volatile bool core1idle = true; // core1 idle flag
volatile bool useUsb = false;   // use usb uart flag
volatile bool awake = true;

/**
 * @brief Core 1 entry point, runs in background
 */
void core1Entry();

int main(void)
{
    // enable watchdog for 200ms, pause on debug = true
    watchdog_enable(DEFAULT_WATCHDOG_DELAY, true);

    stdio_init_all();

    // init system
    userInit();                        // 374us
    xs = Slave(&xp, *_reg.devAddress); ///< Xerxes slave implementation

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
    try
    {
        device.init();
    }
    catch (const std::exception &e)
    {
        xlog_error("Exception in device init: " << e.what());
        _reg.errorSet(ERROR_MASK_DEVICE_INIT);
    }
    catch (...)
    {
        xlog_error("Unknown exception in device init");
        _reg.errorSet(ERROR_MASK_DEVICE_INIT);
    }
    watchdog_update();

    if (useUsb)
    {

        cout << device.getInfoJson() << endl;

        // set to free running mode and calculate statistics for usb uart mode so we can see the values
        _reg.config->bits.freeRun = 1;
        _reg.config->bits.calcStat = 1;
    }

    xlog_debug("Initializing UART");
    // init uart over RS485
    userInitUart();

    xlog_debug("Binding callbacks");
    // bind callbacks, ~204us
    xs.bind(MSGID_PING, unicast(pingCallback));
    xs.bind(MSGID_WRITE, unicast(writeRegCallback));
    xs.bind(MSGID_READ, unicast(readRegCallback));
    xs.bind(MSGID_SYNC, broadcast(syncCallback));
    xs.bind(MSGID_SLEEP, broadcast(sleepCallback));
    xs.bind(MSGID_RESET_SOFT, broadcast(softResetCallback));
    xs.bind(MSGID_RESET_HARD, unicast(factoryResetCallback));
    xs.bind(MSGID_GET_INFO, unicast(getSensorInfoCallback));

    xlog_debug("Draining UART FIFOs");
    // drain uart fifos, just in case there is something in there
    while (!queue_is_empty(&txFifo))
        queue_remove_blocking(&txFifo, NULL);
    while (!queue_is_empty(&rxFifo))
        queue_remove_blocking(&rxFifo, NULL);

    xlog_debug("Starting core 1 for device operation");
    // start core1 for device operation
    multicore_launch_core1(core1Entry);

    xlog_debug("Entering main loop");
    // main loop, runs forever, handles all communication in this loop
    while (1)
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

// save power in release mode
#ifdef NDEBUG
        if (core1idle)
        {
            // setClocksLP();
            sleep_us(10);
            // setClocksHP();
        }
#endif // NDEBUG
    }
}

void core1Entry()
{
    xlog_dbg("Entering core 1");
    uint64_t endOfCycle = 0;
    uint64_t cycleDuration = 0;
    int64_t sleepFor = 0;

    // let core0 lockout core1
    multicore_lockout_victim_init();

// enable gpio interrupts for core1 e.g. for actors or encoders
#if defined(__SHIELD_ENCODER) || defined(__SHIELD_CUTTER)
    // top priority to catch encoder events
    irq_set_priority(IO_IRQ_BANK0, 0);

    // enable gpio interrupts for encoder using lambda function and set callback simultaneously
    gpio_set_irq_enabled_with_callback(
        Xerxes::ENCODER_PIN_A,
        GPIO_IRQ_EDGE_RISE,
        true,
        [](uint gpio, uint32_t)
        {
            device.encoderIrqHandler(gpio);
        });
#endif // __SHIELD_ENCODER

#if defined(__TIGHTLOOP)
    // set core1 to free run mode, process device data as fast as possible
    while (true)
    {
        device.update();
    }

#else  // __TIGHTLOOP
    // core1 mainloop
    while (true)
    {
        // core is set to free run, start cycle
        auto startOfCycle = time_us_64();

        // turn on led for a short time to signal start of cycle
        gpio_put(USR_LED_PIN, 1);
        sleep_us(10);
        // turn off led
        gpio_put(USR_LED_PIN, 0);

        if (_reg.config->bits.freeRun)
        {
            try
            {
                device.update();
            }
            catch (const std::exception &e)
            {
                xlog_error("Exception in device update: " << e.what());
                _reg.errorSet(ERROR_MASK_SENSOR_FAULT);
            }
            catch (...)
            {
                xlog_error("Unknown exception in device update");
                _reg.errorSet(ERROR_MASK_SENSOR_FAULT);
            }
        }

        { // scope for json output
            // cout timestamp and net cycle time in json format
            auto timestamp = time_us_64();
            stringstream ss;
            ss << "{" << endl;
            ss << "\"timestamp\":" << timestamp << "," << endl;
            ss << "\"netCycleTimeUs\":" << *_reg.netCycleTimeUs << "," << endl;
            ss << "\"errors\": 0b" << bitset<32>(*_reg.error) << ",\n";

            // ss device values in json format
            ss << "\"device\":" << device.getJson() << endl;
            ss << "}" << endl;
            printf("%s\n", ss.str().c_str());
        }

        // calculate how long it took to finish cycle
        endOfCycle = time_us_64();
        cycleDuration = endOfCycle - startOfCycle;

        // calculate net cycle time as moving average
        *_reg.netCycleTimeUs = cycleDuration;

        // calculate remaining sleep time
        sleepFor = *_reg.desiredCycleTimeUs - cycleDuration;
        xlog_debug("Cycle duration: " << cycleDuration << "us, sleep for: " << sleepFor << "us");

        uint32_t heap_tot, heap_free, heap_used;
        heap_tot = getTotalHeap();
        heap_free = getFreeHeap();
        heap_used = getUsedHeap();

        xlog_debug("Heap: " << heap_tot / 1024 << "kiB, free: " << heap_free / 1024 << "kiB, used: " << heap_used / 1024 << "kiB");

        // sleep for the remaining time
        if (sleepFor > 0 && sleepFor < *_reg.desiredCycleTimeUs)
        {
            core1idle = true;
            sleep_us(sleepFor);
            core1idle = false;
            _reg.errorClear(ERROR_MASK_SENSOR_OVERLOAD);
        }
        else
        {
            _reg.errorSet(ERROR_MASK_SENSOR_OVERLOAD);
            xlog_debug("Cycle overrun: " << sleepFor << "us");
        }
    }
#endif // __TIGHTLOOP

    core1idle = true;
}