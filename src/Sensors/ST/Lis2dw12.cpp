#include "Sensors/ST/Lis2dw12.hpp"
#include "Utils/Log.h"
#include <iostream>
#include <sstream>

namespace Xerxes
{
    namespace CTRL1
    {
        uint8_t val(ODR odr, MODE mode, LP_MODE lp_mode)
        {
            // Shift each enum to its position and combine using bitwise OR
            return ((uint8_t)odr << 4) | ((uint8_t)mode << 2) | (uint8_t)lp_mode;
        }
    }

    const bool LIS2::dataReady() const
    {
        Status_t status;
        status.reg = readRegister(REG::STATUS);
        return status.bits.DRDY;
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
            xlog_error("LIS2 whoami failed");
            throw std::runtime_error("LIS2 init failed");
        }

        // set CTRL1 register
        writeRegister(REG::CTRL1, CTRL1::val(CTRL1::ODR::ODR_25HZ, CTRL1::MODE::HIGH_PERF, CTRL1::LP_MODE::LP_MODE1));
        uint8_t ctrl1 = readRegister(REG::CTRL1);
        xlog_debug("LIS2 CTRL1: " << (int)ctrl1);

        writeRegister(REG::CTRL6, CTRL6::BW_FILT_ODR_2 | CTRL6::FS_2G | CTRL6::FDS_LOW_PASS | CTRL6::LOW_NOISE_ENABLED);
        uint8_t ctrl6 = readRegister(REG::CTRL6);
        xlog_debug("LIS2 CTRL6: " << (int)ctrl6);

        Status_t status;
        status.reg = readRegister(REG::STATUS);
        xlog_debug("LIS2 STATUS: " << (int)status.reg);
    }

    void LIS2::stop()
    {
        spi_deinit(spi0);
        gpio_set_dir(SPI0_CSN_PIN, GPIO_IN);
    }

    void LIS2::update()
    {
        while (!dataReady())
        {
            sleep_us(1);
        }
        uint8_t x_l = readRegister(REG::OUT_X_L);
        uint8_t x_h = readRegister(REG::OUT_X_H);
        uint8_t y_l = readRegister(REG::OUT_Y_L);
        uint8_t y_h = readRegister(REG::OUT_Y_H);
        uint8_t z_l = readRegister(REG::OUT_Z_L);
        uint8_t z_h = readRegister(REG::OUT_Z_H);
        uint8_t t_l = readRegister(REG::OUT_T_L);
        uint8_t t_h = readRegister(REG::OUT_T_H);
        int16_t x = (int16_t)(x_h << 8 | x_l);
        int16_t y = (int16_t)(y_h << 8 | y_l);
        int16_t z = (int16_t)(z_h << 8 | z_l);
        int16_t t = (int16_t)(t_h << 4 | t_l >> 4);
        *_reg->pv0 = (float)x * 2 / (1 << 15);
        *_reg->pv1 = (float)y * 2 / (1 << 15);
        *_reg->pv2 = (float)z * 2 / (1 << 15);
        *_reg->pv3 = ((float)t / 16.0) + 25;
        std::cout << "[" << time_us_64() << "] x: " << *_reg->pv0 << ", y: " << *_reg->pv1 << ", z: " << *_reg->pv2 << ", t: " << *_reg->pv3 << std::endl;
        float len = sqrtf(*_reg->pv0 * *_reg->pv0 + *_reg->pv1 * *_reg->pv1 + *_reg->pv2 * *_reg->pv2);
        std::cout << "len: " << len << std::endl;

        // super::update();
    }

    std::string LIS2::getJson()
    {
        std::stringstream ss;
        ss << "{";
        ss << "\"x\":" << *_reg->pv0 << ",";
        ss << "\"y\":" << *_reg->pv1 << ",";
        ss << "\"z\":" << *_reg->pv2 << ",";
        ss << "\"t\":" << *_reg->pv3;
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
        sleep_us(1);
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
        xlog_debug("LIS2 readRegister: 0x" << std::hex << (int)reg << ", val: " << (int)data << std::dec);

        return data;
    }

    const bool LIS2::writeRegister(uint8_t reg, uint8_t val) const
    {
        gpio_put(SPI0_CSN_PIN, 0);
        sleep_us(10);
        // send write command
        const uint8_t _cmd = reg | WRITE;
        spi_write_blocking(spi0, &_cmd, 1);

        // write data
        const uint8_t _l = spi_write_blocking(spi0, &val, 1);
        gpio_put(SPI0_CSN_PIN, 1);

        if (_l != 1)
        {
            xlog_error("LIS2 writeRegister failed");
            return false;
        }

        xlog_debug("LIS2 writeRegister: 0x" << std::hex << (int)reg << ", val: " << (int)val << std::dec);

        return true;
    }
} // namespace Xerxes