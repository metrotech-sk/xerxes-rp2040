#include "hx711.hpp"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "Hardware/Board/xerxes_rp2040.h"
#include <sstream>

namespace Xerxes
{

    bool HX711::data()
    {
        return gpio_get(I2C0_SDA_PIN);
    }

    void HX711::clock(bool level)
    {
        gpio_put(I2C0_SCL_PIN, level);
        sleep_us(2);
    }

    /**
     * Read channel A from HX711 IC w/ gain=128 (25 clock pulses)
     * @return
     */
    int32_t HX711::read(void)
    {
        int32_t Count;
        uint8_t i;
        // ADDO = 1;
        clock(0);
        Count = 0;

        // wait for data to be ready
        while (data())
            ;

        for (i = 0; i < 24; i++)
        {
            clock(1);
            Count = Count << 1;
            clock(0);
            if (data())
                Count++;
        }
        clock(1);
        Count = Count ^ 0x800000;
        clock(0);
        Count -= 1L << 23;

        return Count;
    }

    /**
     * @brief Initialize the HX711 ADC
     */
    void HX711::init()

    {
        _devid = DEVID_STRAIN_24BIT;

        // change sample rate to 10Hz
        *_reg->desiredCycleTimeUs = _sensorUpdateRateUs;

        rbpv0 = StatisticBuffer<float>(_sensorFreqHz);

        // turn on 3.3V supply
        gpio_init(EXT_3V3_EN_PIN);
        gpio_set_dir(EXT_3V3_EN_PIN, GPIO_OUT);
        gpio_put(EXT_3V3_EN_PIN, 1);

        // init pins for synchronous serial
        gpio_init(I2C0_SCL_PIN);
        gpio_set_dir(I2C0_SCL_PIN, GPIO_OUT);
        gpio_init(I2C0_SDA_PIN);
        gpio_set_dir(I2C0_SDA_PIN, GPIO_IN);
        gpio_put(I2C0_SCL_PIN, 0);
    }

    void HX711::update()
    {
        // read hx711 adc
        *_reg->pv0 = (this->read() * *_reg->gainPv0) + *_reg->offsetPv0;

        // if calcStat is true, update statistics
        if (_reg->config->bits.calcStat)
        {
            // insert new values into ring buffer
            rbpv0.insertOne(*_reg->pv0);

            // update statistics
            rbpv0.updateStatistics();

            // update min, max stddev etc...
            rbpv0.getStatistics(_reg->minPv0, _reg->maxPv0, _reg->meanPv0, _reg->stdDevPv0);
        }
    }

    void HX711::stop()
    {
        // turn off 3.3V supply
        gpio_put(EXT_3V3_EN_PIN, 0);
    }

    std::string HX711::getJson()
    {
        using namespace std;
        stringstream ss;

        ss << endl
           << "{" << endl;
        ss << "  \"Last\":" << *_reg->pv0 << "," << endl;
        ss << "  \"Min\":" << *_reg->minPv0 << "," << endl;
        ss << "  \"Max\":" << *_reg->maxPv0 << "," << endl;
        ss << "  \"Mean\":" << *_reg->meanPv0 << "," << endl;
        ss << "  \"StdDev\":" << *_reg->stdDevPv0 << endl;

        ss << "}";

        return ss.str();
    }

} // namespace Xerxes
