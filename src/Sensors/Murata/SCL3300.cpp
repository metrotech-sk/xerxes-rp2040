#include "Sensors/Murata/SCL3300.hpp"

#include "Hardware/Board/xerxes_rp2040.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include <sstream>
#include "Core/Errors.h"
#include "Utils/Log.h"

namespace Xerxes
{

    void SCL3300::initSequence()
    {
        // chip init sequence
        volatile uint32_t received_packet;

        sleep_ms(1);
        ExchangeBlock(CMD::Switch_to_bank_0);
        sleep_ms(3);

        ExchangeBlock(CMD::SW_Reset);
        sleep_ms(3);

        ExchangeBlock(CMD::Change_to_mode_4);
        sleep_ms(1);
        ExchangeBlock(CMD::Enable_ANGLE);
        sleep_ms(100);

        ExchangeBlock(CMD::Read_Status_Summary);
        ExchangeBlock(CMD::Read_Status_Summary);
        ExchangeBlock(CMD::Read_Status_Summary);
        sleep_ms(1);
        ExchangeBlock(CMD::Read_WHOAMI);
        ExchangeBlock(CMD::Read_WHOAMI);

        needInit = false;
    }

    void SCL3300::init()
    {
        _devid = DEVID_ANGLE_XY_90;
        _label = "Inclinometer SCL3300 +/- 10°";

        constexpr uint spi_freq = 4 * MHZ;
        // init spi with freq , return actual frequency
        uint baudrate = spi_init(spi0, spi_freq);
        spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

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

        // call init sequence
        initSequence();

        constexpr uint32_t ring_buf_len = 10;
        // initialize ringbuffer with size of RING_BUFFER_LEN (defined in Definitions.h)
        rbpv0 = StatisticBuffer<float>(ring_buf_len);
        rbpv1 = StatisticBuffer<float>(ring_buf_len);
        rbpv2 = StatisticBuffer<float>(ring_buf_len);
        rbpv3 = StatisticBuffer<float>(ring_buf_len);

        // change sample rate to 10Hz
        *_reg->desiredCycleTimeUs = _sensorUpdateRateUs;

        this->update();
    }

    void SCL3300::update()
    {
        // check if init sequence is needed
        if (needInit)
        {
            initSequence();
        }

        // declare packet variables
        auto packetX = std::make_unique<SclPacket_t>();
        auto packetY = std::make_unique<SclPacket_t>();
        // auto packetZ = std::make_unique<SclPacket_t>();  // not used anymore
        auto packetT = std::make_unique<SclPacket_t>();

        // read sensor data from sensor twice because of communication shift
        ExchangeBlock(CMD::Read_ANG_X); // this will read the last value from the previous update
        longToPacket(ExchangeBlock(CMD::Read_ANG_Y), packetX);

        // longToPacket(ExchangeBlock(CMD::Read_ANG_Z), packetY);
        longToPacket(ExchangeBlock(CMD::Read_Temperature), packetY);
        longToPacket(ExchangeBlock(CMD::Read_Status_Summary), packetT);

        // convert data to angles
        *_reg->pv0 = (static_cast<float>(getDegFromPacket(packetX)) * *_reg->gainPv0) - *_reg->offsetPv0;
        *_reg->pv1 = (static_cast<float>(getDegFromPacket(packetY)) * *_reg->gainPv1) - *_reg->offsetPv1;
        // pv2 = static_cast<float>(getDegFromPacket(packetZ));

        if (!packetX->DATA_H &&
            !packetX->DATA_L &&
            !packetY->DATA_H &&
            !packetY->DATA_L)
        {
            // sensor is working but not giving any data - reinit
            this->needInit = true;
        }

        // extract temperature from packet and convert to degrees
        uint16_t raw_temp = (uint16_t)(packetT->DATA_H << 8) + packetT->DATA_L;
        float temp = -273 + (static_cast<float>(raw_temp) / 18.9);
        if (temp < -40 || temp > 85)
        {
            // temperature is out of range
            xlog_warn("Temperature out of range: " << temp);
        }
        else
        {
            *_reg->pv3 = (temp * *_reg->gainPv3) - *_reg->offsetPv3;
        }

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

    double SCL3300::getDegFromPacket(const std::unique_ptr<SclPacket_t> &packet)
    {
        // convert to signed int from 2's complement representation
        int16_t raw = (int16_t)(packet->DATA_H << 8) + (int16_t)packet->DATA_L;

        double degrees = (double)raw * 180 / (1 << 15);
        return degrees;
    }

    std::string SCL3300::getJson()
    {
        using namespace std;

        stringstream ss;

        // return values as JSON
        ss << "{" << endl;
        ss << "\t\"X\":" << *_reg->pv0 << "," << endl;
        ss << "\t\"Y\":" << *_reg->pv1 << "," << endl;
        ss << "\t\"t\":" << *_reg->pv3 << "," << endl;
        ss << "\t\"Avg(X)\":" << *_reg->meanPv0 << "," << endl;
        ss << "\t\"Avg(Y)\":" << *_reg->meanPv1 << "," << endl;
        ss << "\t\"StdDev(X)\":" << *_reg->stdDevPv0 << "," << endl;
        ss << "\t\"StdDev(Y)\":" << *_reg->stdDevPv1 << "," << endl;
        ss << "\t\"Avg(t)\":" << *_reg->meanPv3 << "," << endl;

        ss << "}";

        return ss.str();
    }

} // namespace Xerxes