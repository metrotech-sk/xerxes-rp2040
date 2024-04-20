#include "Sensors/Honeywell/ABP.hpp"

#include "Hardware/Board/xerxes_rp2040.h"
#include "Core/Errors.h"
#include "hardware/spi.h"
#include "pico/time.h"
#include <array>
#include <sstream>
#include <iostream>
#include "Utils/Log.h"

namespace Xerxes
{

    void ABP::stop()
    {
        gpio_put(EXT_3V3_EN_PIN, false);
        spi_deinit(spi0);
        gpio_set_dir(EXT_3V3_EN_PIN, GPIO_IN);
        gpio_set_dir(SPI0_CSN_PIN, GPIO_IN);
    }

    void ABP::init()
    {
        _devid = DEVID_PRESSURE_60MBAR;
        _label = "ABP pressure transducer";

        // init spi with freq 800kHz, return actual frequency
        uint baudrate = spi_init(spi0, 800'000);
        xlog_info("ABP spi init, baudrate: " << baudrate);

        // Set the GPIO pin mux to the SPI
        gpio_set_function(SPI0_MISO_PIN, GPIO_FUNC_SPI);
        gpio_set_function(SPI0_MOSI_PIN, GPIO_FUNC_SPI);
        gpio_set_function(SPI0_CLK_PIN, GPIO_FUNC_SPI);

        gpio_init(EXT_3V3_EN_PIN);
        gpio_set_dir(EXT_3V3_EN_PIN, GPIO_OUT);

        // enable sensor 3V3
        gpio_put(EXT_3V3_EN_PIN, true);

        // CS pin as GPIO
        gpio_init(SPI0_CSN_PIN);
        gpio_set_dir(SPI0_CSN_PIN, GPIO_OUT);

        // read out first sequence - usually rubbish anyway
        sleep_ms(10);
        this->update();
    }

    void ABP::update()
    {

        uint8_t data[4];
        // uint8_t b0, b1, t0, t1;
        uint16_t p_val;
        uint16_t t_val;
        uint8_t status;

        // SPI_CS active low, read data
        gpio_put(SPI0_CSN_PIN, 0);
        sleep_us(3);
        spi_read_blocking(spi0, 0, data, 4);
        sleep_us(3);
        gpio_put(SPI0_CSN_PIN, 1);
        xlog_debug("ABP data: " << (int)data[0] << " " << (int)data[1] << " " << (int)data[2] << " " << (int)data[3]);

        // check if data is ok (no spi error) and set error bit if not
        if (!isSpiDataOk(data, sizeof(data)))
        {
            // set error bit in error register - sensor is not connected
            _reg->errorSet(ERROR_MASK_SENSOR_CONNECTION);
            // set error bit in error register - sensor was not connected in past
            _reg->errorSet(ERROR_MASK_SENSOR_CONNECTION_MEM); // set error flag
        }
        else
        {
            // clear error bit in error register - sensor is connected now
            _reg->errorClear(ERROR_MASK_SENSOR_CONNECTION);
        }

        // convert data, see datasheet
        status = data[0] >> 6;
        p_val = (uint16_t)(((data[0] & 0b00111111) << 8) + data[1]);
        t_val = (uint16_t)(((data[2] << 8) + (data[3] & 0b11100000)) >> 5);

        float pressure_Pa = (float)((((p_val - VALmin) * (Pmax - Pmin)) / (VALmax - VALmin)) + Pmin);
        float temperature = (float)((t_val * 200.0 / 2047.0) - 50.0); // calculate internal temperature
        // calculate density of mono propylene glycol (MPG) at given temperature
        float density = (-1.09010989 * temperature) + 1045.824176;
        // calculate pressure of monopropylene glycol column (density = 1.035 g/cm^3)
        float pressure_mmMPG = 1e3f * pressure_Pa / (density * 9.80665); // calculate pressure in mmMPG
        xlogd("Density: " << density << "[kg/m^3]");

        *_reg->pv0 = pressure_Pa;
        *_reg->pv1 = pressure_mmMPG;
        *_reg->pv3 = temperature;

        xlog_debug("ABP p: " << *_reg->pv0 << "[Pa] = " << *_reg->pv1 << "[mmMPG], t: " << *_reg->pv3 << "[°C]");

        // if calcStat is true, update statistics
        if (_reg->config->bits.calcStat)
        {
            // insert new values into ring buffer
            rbpv0.insertOne(*_reg->pv0);
            rbpv1.insertOne(*_reg->pv1);
            rbpv3.insertOne(*_reg->pv3);

            // update statistics
            rbpv0.updateStatistics();
            rbpv1.updateStatistics();
            rbpv3.updateStatistics();

            // update min, max stddev etc...
            rbpv0.getStatistics(_reg->minPv0, _reg->maxPv0, _reg->meanPv0, _reg->stdDevPv0);
            rbpv1.getStatistics(_reg->minPv1, _reg->maxPv1, _reg->meanPv1, _reg->stdDevPv1);
            rbpv3.getStatistics(_reg->minPv3, _reg->maxPv3, _reg->meanPv3, _reg->stdDevPv3);
        }
    }

    std::string ABP::getJson()
    {
        std::stringstream ss;

        // return values as JSON
        ss << "{" << std::endl;
        ss << "\t\"p[Pa]\": " << *_reg->pv0 << "," << std::endl;
        ss << "\t\"p[mmMPG]\": " << *_reg->pv1 << "," << std::endl;
        ss << "\t\"Avg(t)\": " << *_reg->meanPv3 << "," << std::endl;
        ss << "\t\"Avg(p)\": " << *_reg->meanPv0 << "," << std::endl;
        ss << "\t\"StdDev(p)\": " << *_reg->stdDevPv0 << "," << std::endl;
        ss << "\t\"Min(p)\": " << *_reg->minPv0 << "," << std::endl;
        ss << "\t\"Max(p)\": " << *_reg->maxPv0 << std::endl;
        ss << "}";

        return ss.str();
    }

} // namespace Xerxes