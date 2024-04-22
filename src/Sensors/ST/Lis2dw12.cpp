#include "Sensors/ST/Lis2dw12.hpp"
#include "Utils/Log.h"
#include <iostream>
#include <sstream>
#include "Utils/Various.hpp"
#include "Utils/FFT.hpp"

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
        *_reg->desiredCycleTimeUs = 10000; // 100Hz

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

        CTRL1::ODR ODR;
        switch (FREQ)
        {
        case 100:
            ODR = CTRL1::ODR::ODR_100HZ;
            break;

        case 200:
            ODR = CTRL1::ODR::ODR_200HZ;
            break;

        case 400:
            ODR = CTRL1::ODR::ODR_400_200HZ;
            break;

        case 800:
            ODR = CTRL1::ODR::ODR_800_200HZ;
            break;

        case 1600:
            ODR = CTRL1::ODR::ODR_1600_200HZ;
            break;

        default:
            throw std::runtime_error("Invalid frequency");
            break;
        }

        // set CTRL1 register
        writeRegister(REG::CTRL1, CTRL1::val(ODR, CTRL1::MODE::HIGH_PERF, CTRL1::LP_MODE::LP_MODE1));
        uint8_t ctrl1 = readRegister(REG::CTRL1);
        xlog_debug("LIS2 CTRL1: " << (int)ctrl1);

        writeRegister(REG::CTRL6, CTRL6::BW_FILT_ODR_2 | CTRL6::FS_2G | CTRL6::FDS_LOW_PASS | CTRL6::LOW_NOISE_ENABLED);
        uint8_t ctrl6 = readRegister(REG::CTRL6);
        xlog_debug("LIS2 CTRL6: " << (int)ctrl6);

        Status_t status;
        status.reg = readRegister(REG::STATUS);
        xlog_debug("LIS2 STATUS: " << (int)status.reg);

        // clear message buffer
        float *data = (float *)(_reg->message);
        for (size_t i = 0; i < 64; i++)
        {
            data[i] = 0;
        }
    }

    void LIS2::stop()
    {
        spi_deinit(spi0);
        gpio_set_dir(SPI0_CSN_PIN, GPIO_IN);
    }

    void LIS2::update()
    {
        std::vector<cf> *ptot = new std::vector<cf>(N_SAMPLES);

        auto time_start = time_us_64();

        for (size_t i = 0; i < N_SAMPLES; i++)
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

            int16_t xi = (int16_t)(x_h << 8 | x_l);
            int16_t yi = (int16_t)(y_h << 8 | y_l);
            int16_t zi = (int16_t)(z_h << 8 | z_l);

            float xf, yf, zf;
            xf = (float)xi * 2 / (1 << 15); // unit is 1g = 9.81m.s^-1
            yf = (float)yi * 2 / (1 << 15); // unit is 1g = 9.81m.s^-1
            zf = (float)zi * 2 / (1 << 15); // unit is 1g = 9.81m.s^-1

            float tot = sqrt(xf * xf + yf * yf + zf * zf);

            ptot->at(i) = cf(tot - 1, 0); // -1 to remove DC component

            watchdog_update();
        }

        auto time_end = time_us_64();
        xlog_info("LIS2 update took: " << (time_end - time_start) / 1000 << "ms");
        xlog_info("Used heap: " << std::getUsedHeap() / 1024 << "kiB");

        auto stddev = stddev_signal(ptot);
        xlog_info("Stddev: " << stddev);

        fft(ptot); // around 300ms per 2048 samples

        xlog_info("FFT done, swaping phase for frequency bins.");
        phase_to_freq(ptot, FREQ);
        rectify_fft_output(ptot);

        xlog_info("Removing second half of the spectrum - it is a mirror image of the first half");
        truncate_fft_output(ptot);
        xlog_info("Vec size: " << ptot->size() << ", FREQ: " << FREQ << "Hz");

#ifndef NDEBUG // debug only
        xlog_info("Done, printing");
        print_fft_output(ptot, FREQ, 32);
#endif // NDEBUG

        xlog_info("Sorting FFT output");
        sort_fft_output(ptot); // around 40ms per 2048 samples

#ifndef NDEBUG
        xlog_info("Done, printing");
        print_fft_output(ptot, FREQ, 32);
#endif // !NDEBUG

        uint8_t t_l = readRegister(REG::OUT_T_L);
        uint8_t t_h = readRegister(REG::OUT_T_H);
        int16_t ti = (int16_t)(t_h << 4 | t_l >> 4);
        auto tf = ((float)ti / 16.0) + 25; // unit is °C, resolution 16 LSB/°C = 0.0625°C
        xlog_info("Temperature: " << tf);

        // store sorted FFT output in message buffer
        float *data = (float *)(_reg->message);

        for (size_t i = 0; i < 64; i += 2) // 256bytes / 4bpf = 64 floats
        {
            data[i] = ptot->at(i / 2).imag();     // frequency
            data[i + 1] = ptot->at(i / 2).real(); // magnitude
        }
        xlog_info("Data stored in message buffer");

#ifndef NDEBUG
        // print data to console for debugging
        for (size_t i = 0; i < 64; i += 2) // 256bytes / 4bpf = 64 floats
        {
            std::cout << data[i] << ", " << data[i + 1] << std::endl;
        }
#endif // NDEBUG

        // store top 4 frequencies in pv/av registers
        *_reg->pv0 = ptot->at(0).imag(); // frequency
        *_reg->pv1 = ptot->at(1).imag(); // frequency
        *_reg->pv2 = ptot->at(2).imag(); // frequency
        *_reg->pv3 = ptot->at(3).imag(); // frequency

        *_reg->av0 = ptot->at(0).real(); // magnitude
        *_reg->av1 = ptot->at(1).real(); // magnitude
        *_reg->av2 = ptot->at(2).real(); // magnitude
        *_reg->av3 = ptot->at(3).real(); // magnitude

        delete ptot;

        // super::update();
    }

    std::string LIS2::getJson()
    {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"f0\":" << *_reg->pv0 << ",\n";
        ss << "  \"f1\":" << *_reg->pv1 << ",\n";
        ss << "  \"f2\":" << *_reg->pv2 << ",\n";
        ss << "  \"f3\":" << *_reg->pv3 << ",\n";
        ss << "  \"m0\":" << *_reg->av0 << ",\n";
        ss << "  \"m1\":" << *_reg->av1 << ",\n";
        ss << "  \"m2\":" << *_reg->av2 << ",\n";
        ss << "  \"m3\":" << *_reg->av3 << "\n";
        ss << "  }" << std::endl;
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