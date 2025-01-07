#include "Utils/Gpio.hpp"
#include "pico/stdlib.h"
#include "Hardware/Board/xerxes_rp2040.h"

namespace Xerxes
{
    void ledBusyOn()
    {
        gpio_put(USR_LED_PIN, 1);
    }

    void ledBusyOff()
    {
        gpio_put(USR_LED_PIN, 0);
    }

    void comOn()
    {
        gpio_put(LED_COM_ACT_PIN, 1);
    }

    void comOff()
    {
        gpio_put(LED_COM_ACT_PIN, 0);
    }

    void enableTX()
    {
        gpio_put(RS_EN_PIN, 1);
    }

    void disableTX()
    {
        gpio_put(RS_EN_PIN, 0);
    }
} // namespace Xerxes