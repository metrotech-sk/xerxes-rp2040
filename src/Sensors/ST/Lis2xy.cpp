#include "Sensors/ST/Lis2xy.hpp"
#include "Utils/Log.h"
#include <iostream>
#include <sstream>
#include "Utils/Various.hpp"

namespace Xerxes
{
    void LIS2XY::init()
    {
        super::init();

        xlog_info("LIS2XY init");
        _devid = DEVID_ACCEL_LIS_XY;
        _label = "LIS2DW12 Accelerometer in XY axis";

        delete frequencies;
        delete amplitudes;

        amplitudes_x = new std::array<float, N_SAMPLES / 2>();  // Allocate on the heap
        frequencies_x = new std::array<float, N_SAMPLES / 2>(); // Allocate on the heap
        amplitudes_y = new std::array<float, N_SAMPLES / 2>();  // Allocate on the heap
        frequencies_y = new std::array<float, N_SAMPLES / 2>(); // Allocate on the heap
        xlog_debug("LIS2XY arrays initialized");
    }

    void LIS2XY::stop()
    {
        spi_deinit(spi0);
        gpio_set_dir(SPI0_CSN_PIN, GPIO_IN);

        // Ensure that memory is properly freed in stop
        if (ptot_x)
            delete ptot_x;
        ptot_x = nullptr;

        if (ptot_y)
            delete ptot_y;
        ptot_y = nullptr;

        if (ampls_x)
            delete ampls_x;
        ampls_x = nullptr;

        if (ampls_y)
            delete ampls_y;
        ampls_y = nullptr;

        if (frequencies_x)
            delete frequencies_x;
        frequencies_x = nullptr;

        if (frequencies_y)
            delete frequencies_y;
        frequencies_y = nullptr;

        if (amplitudes_x)
            delete amplitudes_x;
        amplitudes_x = nullptr;

        if (amplitudes_y)
            delete amplitudes_y;
        amplitudes_y = nullptr;
    }

    void LIS2XY::update()
    {
        xlog_debug("LIS2 update start");

        auto time_start = time_us_64();
        uint32_t drdyctr = 0;

        ptot_x = new std::vector<cf>(N_SAMPLES);     // Allocate on the heap
        ampls_x = new std::vector<float>(N_SAMPLES); // Allocate on the heap
        ptot_y = new std::vector<cf>(N_SAMPLES);     // Allocate on the heap
        ampls_y = new std::vector<float>(N_SAMPLES); // Allocate on the heap
        xlog_debug("LIS2XY vectors initialized");

        ptot_x->resize(N_SAMPLES);
        ptot_y->resize(N_SAMPLES);
        ampls_x->resize(N_SAMPLES);
        ampls_y->resize(N_SAMPLES);
        xlog_debug("LIS2XY vectors resized");

        // erase amplitudes_x with zeros:
        std::fill(amplitudes_x->begin(), amplitudes_x->end(), 0);
        xlog_debug("LIS2XY array x zeroed");
        std::fill(amplitudes_y->begin(), amplitudes_y->end(), 0);
        xlog_debug("LIS2XY array y zeroed");

        if (whoami() != 0x44)
        {
            xlog_error("LIS2 whoami failed");
            throw std::runtime_error("LIS2 sensor not connected");
        }
        xlog_debug("LIS2 whoami passed");

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

            // float tot = sqrt(xf * xf + yf * yf + zf * zf);
            // ampls_x->at(i) = tot - 1;
            ampls_x->at(i) = xf;
            ampls_y->at(i) = yf;

            ptot_x->at(i) = cf(xf, 0);
            ptot_y->at(i) = cf(yf, 0);
            // xlog_trace("g total: " << tot << "[g]");

            gpio_put(USR_LED_PIN, 0);

            watchdog_update();
        }
        xlog_debug("Data ready after: " << drdyctr << "us");

        auto time_end = time_us_64();
        xlog_debug("LIS2 update took: " << (time_end - time_start) / 1000 << "ms");

        auto stddev = stddev_signal(ptot_x);
        xlog_debug("Stddev: " << stddev);

        xlog_debug("Memory after measurement: " << getFreeHeap() << "B free, " << getUsedHeap() << "B used, " << getTotalHeap() << "B total");

        fft(ptot_x); // around 300ms per 2048 samples
        fft(ptot_y); // around 300ms per 2048 samples

        xlog_debug("FFT done, swaping phase for frequency bins.");
        phase_to_freq(ptot_x, FREQ);
        phase_to_freq(ptot_y, FREQ);
        xlog_debug("Rectifying FFT output");
        rectify_fft_output(ptot_x);
        rectify_fft_output(ptot_y);

        xlog_debug("Removing second half of the spectrum - it is a mirror image of the first half");
        truncate_fft_output(ptot_x);
        truncate_fft_output(ptot_y);
        xlog_debug("Vec size: " << ptot_x->size() << ", FREQ: " << FREQ << "Hz");
        xlog_debug("Vec size: " << ptot_y->size() << ", FREQ: " << FREQ << "Hz");

#if LOG_LEVEL >= LOG_LEVEL_DEBUG // print only in debug mode
        xlog_info("Done, printing, X");
        print_fft_output(ptot_x, FREQ, 32);
        xlog_info("Done, printing, Y");
        print_fft_output(ptot_y, FREQ, 32);
#endif // NDEBUG
        xlog_debug("Removing DC component");
        ptot_x->at(0) = cf(0, 0); // remove DC component
        ptot_y->at(0) = cf(0, 0); // remove DC component
        xlog_debug("Calculating carrier frequency");
        carrier_x = carrier_freq(ptot_x, 5);
        carrier_y = carrier_freq(ptot_y, 5);
        xlog_info("Carrier frequency: " << carrier_x << "Hz");
        xlog_info("Carrier frequency: " << carrier_y << "Hz");

        // calculate max amplitude
        auto it = std::max_element(ampls_x->begin(), ampls_x->end());
        float max_amplitude = *it;
        xlog_info("Max amplitude: " << max_amplitude << "g, " << max_amplitude * 9.81 << "m.s^-2");

        it = std::max_element(ampls_y->begin(), ampls_y->end());
        max_amplitude = *it;
        xlog_info("Max amplitude: " << max_amplitude << "g, " << max_amplitude * 9.81 << "m.s^-2");

        for (size_t i = 0; i < N_SAMPLES / 2; i++)
        {
            frequencies_x->at(i) = ptot_x->at(i).imag();
            amplitudes_x->at(i) = ptot_x->at(i).real();
            frequencies_y->at(i) = ptot_y->at(i).imag();
            amplitudes_y->at(i) = ptot_y->at(i).real();
        }

        xlog_debug("Sorting FFT output");
        sort_fft_output(ptot_x); // around 40ms per 2048 samples
        sort_fft_output(ptot_y); // around 40ms per 2048 samples

#if LOG_LEVEL >= LOG_LEVEL_INFO
        xlog_info("Done, printing...");
        print_fft_output(ptot_x, FREQ, 32);
        print_fft_output(ptot_y, FREQ, 32);
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

        xlog_debug("Memory before message storage: " << getFreeHeap() << "B free, " << getUsedHeap() << "B used, " << getTotalHeap() << "B total");

        for (size_t i = 0; i < 128; i += 2) // 1024 bytes / 4bpf = 256 floats
        {
            data[i] = ptot_x->at(i / 2).imag();     // frequency
            data[i + 1] = ptot_x->at(i / 2).real(); // magnitude
            xlog_debug("fx: " << data[i] << ", mx: " << data[i + 1] << " at " << i);
        }
        xlog_debug("Data X stored in message buffer");

        delete ptot_x;
        delete ampls_x;

        for (size_t i = 0; i < 128; i += 2) // 1024 bytes / 4bpf = 256 floats
        {
            data[i + 128] = ptot_y->at(i / 2).imag(); // frequency
            data[i + 129] = ptot_y->at(i / 2).real(); // magnitude
            xlog_debug("fy: " << data[i + 128] << ", my: " << data[i + 129] << " at " << i + 128);
        }

        xlog_debug("LIS2 update end, releasing allocated memory");
        delete ptot_y;
        delete ampls_y;
        xlog_debug("Memory after delete: " << getFreeHeap() << "B free, " << getUsedHeap() << "B used, " << getTotalHeap() << "B total");

        // super::update();
    }

    std::string LIS2XY::getJson()
    {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"x\":" << *_reg->pv0 << ",\n";
        ss << "  \"y\":" << *_reg->pv1 << ",\n";
        ss << "  \"z\":" << *_reg->pv2 << ",\n";
        ss << "  \"t\":" << *_reg->pv3 << ",\n";
        ss << "  \"carrier_x\":" << carrier_x << ",\n";
        ss << "  \"carrier_y\":" << carrier_y << "\n";
        ss << "}" << std::endl;
        return ss.str();
    }

} // namespace Xerxes