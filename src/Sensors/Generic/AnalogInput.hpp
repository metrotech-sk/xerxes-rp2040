#ifndef __ANALOG_INPUT
#define __ANALOG_INPUT

#include "Sensors/Sensor.hpp"
#include <string>

namespace Xerxes
{

    /// @brief default number of oversampling bits
    constexpr uint8_t defaultOversampleBits = 3;
    /// @brief default number of used ADC channels
    constexpr uint8_t defaultChannels = 4;
    /// @brief bit depth of RP2040 ADC
    constexpr uint8_t rpBitDepth = 12;

    /**
     * @brief Analog input sensor class
     *
     * @details AnalogInput is a generic class for up to 4 analog inputs, e.g. 4x 0-3.3V
     *
     * @note default ADC depth on RP2040 is 12 bit, 8.7 ENOB (approx. 54dB SNR)
     * @note AnalogInput uses oversampling to increase resolution, increasing SNR by 6dB per bit
     * @note n-bit oversampling increases sampling time too: sample time = 4^n * conversion time
     */
    class AnalogInput : public Sensor
    {
    private:
        uint64_t results[4] = {0, 0, 0, 0}; // up to 4 channels, 64 bit to avoid overflow
        // sensor specific variables                              // oversampling bits
        uint8_t oversampleExtraBits = defaultOversampleBits;            // extra bits for oversampling, 3
        uint16_t overSample = 1 << (2 * defaultOversampleBits);         // oversampling factor, 1 << 6 = 64
        uint8_t effectiveBitDepth = rpBitDepth + defaultOversampleBits; // effective bit depth, 12 + 3 = 15
        uint64_t numCounts = 1 << effectiveBitDepth;                    // number of counts, 2^15 = 32768
        uint8_t numChannels = 4;                                        // number of channels, default is 4

    protected:
        // typedef Sensor as super class for easier access
        typedef Sensor super;

        constexpr static uint32_t _updateRateHz = 100;                    // update frequency in Hz
        constexpr static uint32_t _updateRateUs = _usInS / _updateRateHz; // update rate in microseconds

    public:
        using Sensor::Sensor;

        ~AnalogInput();

        void init();

        /**
         * @brief init sensor, set pins as input, init ADC
         *
         */
        void init(uint8_t numChannels, uint8_t oversampleBits = defaultOversampleBits);

        /**
         * @brief update sensor values, takes approx 1ms for 4 channels with 100 samples
         *
         */
        void update();

        /**
         * @brief Not needed for 4xAI
         *
         */
        void stop();

        /**
         * @brief Get the Json object representing the sensor values
         *
         * @return std::string
         */
        std::string getJson();

        std::string getJson(uint8_t channel);
        std::string getJsonLast();
        std::string getJsonMin();
        std::string getJsonMax();
        std::string getJsonMean();
        std::string getJsonStdDev();
    };

} // namespace Xerxes

#endif // !__ANALOG_INPUT