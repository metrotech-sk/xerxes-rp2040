#include "AnalogInput.hpp"

#include "Hardware/Board/xerxes_rp2040.h"
#include "hardware/adc.h"
#include <string>
#include <sstream>
#include "pico/time.h"


namespace Xerxes
{


AnalogInput::~AnalogInput()
{
    this->stop();
}


void AnalogInput::init()
{
    this->init(defaultChannels, defaultOversampleBits);
}


void AnalogInput::init(uint8_t numChannels, uint8_t oversampleBits)
{
    _devid = DEVID_IO_4AI;  // device id
    
    this->oversampleExtraBits = oversampleBits;
    this->numChannels = numChannels;
    this->overSample = 1 << (2 * oversampleBits);
    this->effectiveBitDepth = rpBitDepth + oversampleBits;
    this->numCounts = 1 << effectiveBitDepth;

    // init ADC
    adc_init();
    
    // set pins as input
    adc_gpio_init(ADC0_PIN);
    adc_gpio_init(ADC1_PIN);
    adc_gpio_init(ADC2_PIN);
    adc_gpio_init(ADC3_PIN);

    // set update rate
    *_reg->desiredCycleTimeUs = _updateRateUs;

    // enable power supply to sensor
    gpio_init(EXT_3V3_EN_PIN);
    gpio_set_dir(EXT_3V3_EN_PIN, GPIO_OUT);

    // update sensor values
    this->update();
}


void AnalogInput::update()
{    

    // enable sensor 3V3
    gpio_put(EXT_3V3_EN_PIN, true);
    sleep_us(1000); // wait for sensor to power up
    
    // oversample and average over 4 channels, effectively increasing bit depth by 4 bits
    // https://www.silabs.com/documents/public/application-notes/an118.pdf
    for(uint8_t channel = 0; channel < numChannels; channel++)
    {
        // set channel
        adc_select_input(channel);
        results[channel] = 0;

        // read samples and average
        for(uint16_t i = 0; i < overSample; i++)
        {
            results[channel] += adc_read();
        }

        // right shift by oversampleExtraBits to decimate oversampled bits
    // nothing to do here

        results[channel] >>= oversampleExtraBits;
    }

    // disable sensor 3V3 effectively saving 3mA of current per sensor (1kOhm)
    gpio_put(EXT_3V3_EN_PIN, false);
    
    // convert to value on scale <0, 1)
    // optimization: use if-else instead of switch, since numChannels is known at compile time
    if(numChannels == 1)
    {
        *_reg->pv0 = results[0] / static_cast<double>(numCounts);
    }
    else if(numChannels == 2)
    {
        *_reg->pv0 = results[0] / static_cast<double>(numCounts);
        *_reg->pv1 = results[1] / static_cast<double>(numCounts);
    }
    else if(numChannels == 3)
    {
        *_reg->pv0 = results[0] / static_cast<double>(numCounts);
        *_reg->pv1 = results[1] / static_cast<double>(numCounts);
        *_reg->pv2 = results[2] / static_cast<double>(numCounts);
    }
    else if(numChannels == 4)
    {
        *_reg->pv0 = results[0] / static_cast<double>(numCounts);
        *_reg->pv1 = results[1] / static_cast<double>(numCounts);
        *_reg->pv2 = results[2] / static_cast<double>(numCounts);
        *_reg->pv3 = results[3] / static_cast<double>(numCounts);
    }
    else
    {
        // do nothing
    }


    // if calcStat is true, update statistics
    if(_reg->config->bits.calcStat)
    {
        // insert new values into ring buffer
        rbpv0.insertOne(*_reg->pv0);

        // update statistics
        rbpv0.updateStatistics();

        // update min, max stddev etc...
        rbpv0.getStatistics(_reg->minPv0, _reg->maxPv0, _reg->meanPv0, _reg->stdDevPv0);
    }

    // if calcStat is true and numChannels > 1, update statistics for pv1
    if(_reg->config->bits.calcStat && numChannels > 1)
    {
        rbpv1.insertOne(*_reg->pv1);
        rbpv1.updateStatistics();
        rbpv1.getStatistics(_reg->minPv1, _reg->maxPv1, _reg->meanPv1, _reg->stdDevPv1);
    }

    // if calcStat is true and numChannels > 2, update statistics for pv2
    if(_reg->config->bits.calcStat && numChannels > 2)
    {
        rbpv2.insertOne(*_reg->pv2);
        rbpv2.updateStatistics();
        rbpv2.getStatistics(_reg->minPv2, _reg->maxPv2, _reg->meanPv2, _reg->stdDevPv2);
    }

    // if calcStat is true and numChannels > 3, update statistics for pv3
    if(_reg->config->bits.calcStat && numChannels > 3)
    {
        rbpv3.insertOne(*_reg->pv3);
        rbpv3.updateStatistics();
        rbpv3.getStatistics(_reg->minPv3, _reg->maxPv3, _reg->meanPv3, _reg->stdDevPv3);
    }
}


void AnalogInput::stop()
{
    // disable sensor 3V3
    gpio_put(EXT_3V3_EN_PIN, false);
}


std::string AnalogInput::getJsonLast()
{
    using namespace std;
    stringstream ss;

    ss << endl << "  {" << endl;
    ss << "    \"AI0\": " << *this->_reg->pv0 << "," << endl;
    ss << "    \"AI1\": " << *this->_reg->pv1 << "," << endl;
    ss << "    \"AI2\": " << *this->_reg->pv2 << "," << endl;
    ss << "    \"AI3\": " << *this->_reg->pv3 << endl;    
    ss << "  }";

    return ss.str();
}


std::string AnalogInput::getJsonMin()
{
    using namespace std;
    stringstream ss;

    ss << endl << "  {" << endl;
    ss << "    \"Min(AI0)\": " << *this->_reg->minPv0 << "," << endl;
    ss << "    \"Min(AI1)\": " << *this->_reg->minPv1 << "," << endl;
    ss << "    \"Min(AI2)\": " << *this->_reg->minPv2 << "," << endl;
    ss << "    \"Min(AI3)\": " << *this->_reg->minPv3 << endl;    
    ss << "  }";

    return ss.str();
}


std::string AnalogInput::getJsonMax()
{
    using namespace std;
    stringstream ss;

    ss << endl << "  {" << endl;
    ss << "    \"Max(AI0)\": " << *this->_reg->maxPv0 << "," << endl;
    ss << "    \"Max(AI1)\": " << *this->_reg->maxPv1 << "," << endl;
    ss << "    \"Max(AI2)\": " << *this->_reg->maxPv2 << "," << endl;
    ss << "    \"Max(AI3)\": " << *this->_reg->maxPv3 << endl;    
    ss << "  }";

    return ss.str();
}


std::string AnalogInput::getJsonMean()
{
    using namespace std;
    stringstream ss;

    ss << endl << "  {" << endl;
    ss << "    \"Mean(AI0)\": " << *this->_reg->meanPv0 << "," << endl;
    ss << "    \"Mean(AI1)\": " << *this->_reg->meanPv1 << "," << endl;
    ss << "    \"Mean(AI2)\": " << *this->_reg->meanPv2 << "," << endl;
    ss << "    \"Mean(AI3)\": " << *this->_reg->meanPv3 << endl;    
    ss << "  }";

    return ss.str();
}


std::string AnalogInput::getJsonStdDev()
{
    using namespace std;
    stringstream ss;

    ss << endl << "  {" << endl;
    ss << "    \"StdDev(AI0)\": " << *this->_reg->stdDevPv0 << "," << endl;
    ss << "    \"StdDev(AI1)\": " << *this->_reg->stdDevPv1 << "," << endl;
    ss << "    \"StdDev(AI2)\": " << *this->_reg->stdDevPv2 << "," << endl;
    ss << "    \"StdDev(AI3)\": " << *this->_reg->stdDevPv3 << endl;
    ss << "  }";

    return ss.str();
}


std::string AnalogInput::getJson()
{
    using namespace std;
    stringstream ss;

    ss << endl << "{" << endl;
    ss << "  \"Last\":" << getJsonLast() << "," << endl;
    ss << "  \"Min\":" << getJsonMin() << "," << endl;
    ss << "  \"Max\":" << getJsonMax() << "," << endl;
    ss << "  \"Mean\":" << getJsonMean() << "," << endl;
    ss << "  \"StdDev\":" << getJsonStdDev() << endl;
    
    ss << "}";

    return ss.str();
}


}   // namespace Xerxes