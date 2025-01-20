#include "Sensors/ST/Lis2dw12.hpp"
#include "Utils/Log.h"
#include <iostream>
#include <sstream>
#include "Utils/Various.hpp"

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
        _devid = DEVID_ACCEL_LIS;
        _label = "LIS2DW12 Accelerometer";
        *_reg->desiredCycleTimeUs = 10000; // 100Hz

        // init spi with freq 8MHz, return actual frequency
        constexpr uint desired_freq = 8'000'000;
        uint actual_freq = spi_init(spi0, desired_freq);
        xlog_debug("LIS2 spi init, actual_freq: " << actual_freq);

        // Set the GPIO pin mux to the SPI
        gpio_set_function(SPI0_MISO_PIN, GPIO_FUNC_SPI);
        gpio_set_function(SPI0_MOSI_PIN, GPIO_FUNC_SPI);
        gpio_set_function(SPI0_CLK_PIN, GPIO_FUNC_SPI);

        // CS pin as GPIO
        gpio_init(SPI0_CSN_PIN);
        gpio_set_dir(SPI0_CSN_PIN, GPIO_OUT);

        // Initialize heap-allocated structures in the constructor or init
        frequencies = new std::array<float, N_SAMPLES / 2>; // Allocate on the heap
        amplitudes = new std::array<float, N_SAMPLES / 2>;  // Allocate on the heap

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

        // Ensure that memory is properly freed in stop
        if (ptot)
            delete ptot;
        ptot = nullptr;

        if (ampls)
            delete ampls;
        ampls = nullptr;

        if (frequencies)
            delete frequencies;
        frequencies = nullptr;

        if (amplitudes)
            delete amplitudes;
        amplitudes = nullptr;
    }

    void LIS2::update()
    {
        xlog_debug("LIS2 update start");

        auto time_start = time_us_64();
        uint32_t drdyctr = 0;

        ptot = new std::vector<cf>(N_SAMPLES);     // Allocate on the heap
        ampls = new std::vector<float>(N_SAMPLES); // Allocate on the heap

        ptot->resize(N_SAMPLES);
        ampls->resize(N_SAMPLES);

        // erase amplitudes with zeros:
        std::fill(amplitudes->begin(), amplitudes->end(), 0);

        if (whoami() != 0x44)
        {
            xlog_error("LIS2 whoami failed");
            throw std::runtime_error("LIS2 sensor not connected");
        }

        xlog_debug("Gathering " << N_SAMPLES << " samples");
        for (size_t i = 0; i < N_SAMPLES; i++)
        {
            drdyctr = 0;
            while (!dataReady() && drdyctr < 1000) // wait for data ready signal for max 10ms
            {
                busy_wait_us_32(10);
                drdyctr++;
            }

            gpio_put(USR_LED_PIN, 1); // turn on CPU LED to signal data ready
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

            *_reg->pv0 = xf;
            *_reg->pv1 = yf;
            *_reg->pv2 = zf;
            xlog_trace("XYZ values: " << xf << ", " << yf << ", " << zf << "[g]");

            float tot = sqrt(xf * xf + yf * yf + zf * zf);
            ampls->at(i) = tot - 1; // -1 to remove DC component

            ptot->at(i) = cf(tot - 1, 0); // -1 to remove DC component
            xlog_trace("g total: " << tot << "[g]");

            gpio_put(USR_LED_PIN, 0);

            watchdog_update();
        }
        xlog_debug("Data ready after: " << drdyctr << "us");

        auto time_end = time_us_64();
        xlog_debug("LIS2 update took: " << (time_end - time_start) / 1000 << "ms");

        auto stddev = stddev_signal(ptot);
        xlog_debug("Stddev: " << stddev);

        xlog_debug("Memory after measurement: " << getFreeHeap() << "B free, " << getUsedHeap() << "B used, " << getTotalHeap() << "B total");

        fft(ptot); // around 300ms per 2048 samples

        xlog_debug("FFT done, swaping phase for frequency bins.");
        phase_to_freq(ptot, FREQ);
        xlog_debug("Rectifying FFT output");
        rectify_fft_output(ptot);

        xlog_debug("Removing second half of the spectrum - it is a mirror image of the first half");
        truncate_fft_output(ptot);
        xlog_debug("Vec size: " << ptot->size() << ", FREQ: " << FREQ << "Hz");

#if LOG_LEVEL >= LOG_LEVEL_DEBUG // print only in debug mode
        xlog_info("Done, printing");
        print_fft_output(ptot, FREQ, 32);
#endif // NDEBUG
        xlog_debug("Removing DC component");
        ptot->at(0) = cf(0, 0); // remove DC component
        xlog_debug("Calculating carrier frequency");
        carrier = carrier_freq(ptot, 5);
        xlog_info("Carrier frequency: " << carrier << "Hz");

        // calculate max amplitude
        auto it = std::max_element(ampls->begin(), ampls->end());
        float max_amplitude = *it;
        max_amplitude = max_amplitude; // remove DC component
        xlog_info("Max amplitude: " << max_amplitude << "g, " << max_amplitude * 9.81 << "m.s^-2");

        for (size_t i = 0; i < N_SAMPLES / 2; i++)
        {
            frequencies->at(i) = ptot->at(i).imag();
            amplitudes->at(i) = ptot->at(i).real();
        }

        xlog_debug("Sorting FFT output");
        sort_fft_output(ptot); // around 40ms per 2048 samples

#if LOG_LEVEL >= LOG_LEVEL_INFO
        xlog_info("Done, printing");
        print_fft_output(ptot, FREQ, 32);
#endif // !NDEBUG
        xlog_debug("Reading temperature register");
        uint8_t t_l = readRegister(REG::OUT_T_L);
        uint8_t t_h = readRegister(REG::OUT_T_H);
        int16_t ti = (int16_t)(t_h << 4 | t_l >> 4);
        auto tf = ((float)ti / 16.0) + 25; // unit is °C, resolution 16 LSB/°C = 0.0625°C
        xlog_info("Temperature: " << tf);
        *_reg->pv3 = tf;

        xlog_debug("Data ready, storing in message buffer");
        // store sorted FFT output in message buffer
        float *data = (float *)(_reg->message);

        for (size_t i = 0; i < 256; i += 2) // 1024 bytes / 4bpf = 256 floats
        {
            data[i] = ptot->at(i / 2).imag();     // frequency
            data[i + 1] = ptot->at(i / 2).real(); // magnitude
        }
        xlog_debug("Data stored in message buffer");

        // add carry to message buffer
        data[254] = carrier;
        data[255] = max_amplitude;
        xlog_debug("Carrier and max amplitude stored in message buffer");
        xlog_debug("Carrier: " << data[254] << "Hz, max amplitude: " << data[255] << "g");

        xlog_debug("LIS2 update end, releasing allocated memory");

        delete ptot;
        delete ampls;
        xlog_debug("Memory after delete: " << getFreeHeap() << "B free, " << getUsedHeap() << "B used, " << getTotalHeap() << "B total");

        // super::update();
    }

    std::string LIS2::getJson()
    {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"x\":" << *_reg->pv0 << ",\n";
        ss << "  \"y\":" << *_reg->pv1 << ",\n";
        ss << "  \"z\":" << *_reg->pv2 << ",\n";
        ss << "  \"t\":" << *_reg->pv3 << ",\n";
        ss << "  \"carrier\":" << carrier << ",\n";
        ss << "  \"fft\": {\n";
        if (carrier > 0)
        {
            ss << "    \"frequencies\": [";
            for (size_t i = 0; i < N_SAMPLES / 2; i++)
            {
                ss << frequencies->at(i);
                if (i < N_SAMPLES / 2 - 1)
                {
                    ss << ", ";
                }
            }
            ss << "],\n";
            ss << "    \"amplitudes\": [";
            for (size_t i = 0; i < N_SAMPLES / 2; i++)
            {
                ss << amplitudes->at(i);
                if (i < N_SAMPLES / 2 - 1)
                {
                    ss << ", ";
                }
            }
            ss << "]\n";
        }
        else
        {
            ss << "    \"frequencies\": [],\n";
            ss << "    \"amplitudes\": []\n";
        }

        ss << "  }\n";
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
        // xlog_trace("LIS2 readRegister: 0x" << std::hex << (int)reg << ", val: " << (int)data << std::dec);

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

        // xlog_trace("LIS2 writeRegister: 0x" << std::hex << (int)reg << ", val: " << (int)val << std::dec);

        return true;
    }
} // namespace Xerxes