#ifndef __GPIO_HPP
#define __GPIO_HPP

#include "pico/stdlib.h"

namespace Xerxes
{
    /// @brief Turn on the busy LED
    void ledBusyOn();

    /// @brief Turn off the busy LED
    void ledBusyOff();

    /// @brief Turn on the communication LED
    void ledComOn();

    /// @brief Turn off the communication LED
    void ledComOff();
} // namespace Xerxes

#endif // __GPIO_HPP