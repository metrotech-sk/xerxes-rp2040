#include "Sensors/ST/Lis2dw12.hpp"
#include "Utils/Log.h"
#include <iostream>
#include <sstream>

namespace Xerxes
{
    namespace CTRL1
    {
        uint8_t val(ODR_t odr, MODE_t mode, LP_MODE_t lp_mode)
        {
            // Shift each enum to its position and combine using bitwise OR
            return ((uint8_t)odr << 4) | ((uint8_t)mode << 2) | (uint8_t)lp_mode;
        }
    }

    void LIS2::init()
    {
        xlog_info("LIS2 init");
        _devid = DEVID_PRESSURE_60MBAR;
        _label = "LIS2DW12 Accelerometer";

        // init spi with freq 8MHz, return actual frequency
        constexpr uint desired_freq = 8'000'000;
        uint actual_freq = spi_init(spi0, desired_freq);
        xlog_info("LIS2 spi init, actual_freq: " << actual_freq);

        // Set the GPIO pin mux to the SPI
        gpio_set_function(SPI0_MISO_PIN, GPIO_FUNC_SPI);
        gpio_set_function(SPI0_MOSI_PIN, GPIO_FUNC_SPI);
        gpio_set_function(SPI0_CLK_PIN, GPIO_FUNC_SPI);

        // CS pin as GPIO
        gpio_init(SPI0_CSN_PIN);
        gpio_set_dir(SPI0_CSN_PIN, GPIO_OUT);

        // wait for sensor to power up
        sleep_ms(20);
        if (whoami() != 0x44)
        {
            xlog_error("LIS2 init failed");
            // restart
            throw std::runtime_error("LIS2 init failed");
        }
    }

    void LIS2::stop()
    {
        spi_deinit(spi0);
        gpio_set_dir(SPI0_CSN_PIN, GPIO_IN);
    }

    void LIS2::update() {}

    std::string LIS2::getJson()
    {
        std::stringstream ss;
        ss << "{";
        ss << "whoami: " << whoami();
        ss << "}" << std::endl;
        return ss.str();
    }

    const uint8_t LIS2::whoami() const
    {
        return readRegister(REG::WHO_AM_I);
    }

    const uint8_t LIS2::readRegister(uint8_t reg) const
    {
        gpio_put(SPI0_CSN_PIN, 0);
        sleep_us(10);
        // send read command
        const uint8_t _cmd = reg | READ;
        spi_write_blocking(spi0, &_cmd, 1);

        // read data
        uint8_t data;
        uint8_t _l = spi_read_blocking(spi0, 0, &data, 1);

        gpio_put(SPI0_CSN_PIN, 1);
        if (_l != 1)
        {
            xlog_error("LIS2 readRegister failed");
        }
        xlog_debug("LIS2 readRegister: " << std::hex << (int)data << std::dec);

        return data;
    }
} // namespace Xerxes