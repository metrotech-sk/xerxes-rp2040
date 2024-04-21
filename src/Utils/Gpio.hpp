#ifndef __GPIO_HPP
#define __GPIO_HPP

#include "pico/stdlib.h"

namespace Xerxes
{
    /// @brief Turn on the busy LED
    void ledBusyOn();

    /// @brief Turn off the busy LED
    void ledBusyOff();
} // namespace Xerxes

#endif // __GPIO_HPP