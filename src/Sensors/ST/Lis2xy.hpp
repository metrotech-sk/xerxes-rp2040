#ifndef __LIS2XY_HPP
#define __LIS2XY_HPP

#include "Sensors/Sensor.hpp"
#include "Utils/FFT.hpp"
#include "Sensors/ST/Lis2dw12.hpp"

namespace Xerxes
{
    class LIS2XY : public LIS2
    {
    protected:
        constexpr static size_t N_SAMPLES = 2048;
        constexpr static uint16_t FREQ = 100;

        // must be a vector because of FFT algorithm requirements - resizing is not allowed in array
        std::vector<cf> *ptot_x;
        std::vector<float> *ampls_x;
        std::array<float, N_SAMPLES / 2> *frequencies_x;
        std::array<float, N_SAMPLES / 2> *amplitudes_x;

        std::vector<cf> *ptot_y;
        std::vector<float> *ampls_y;
        std::array<float, N_SAMPLES / 2> *frequencies_y;
        std::array<float, N_SAMPLES / 2> *amplitudes_y;

        float carrier_x;
        float carrier_y;

        // typedef Sensor as super class for easier access
        typedef LIS2 super;

    public:
        using LIS2::LIS2;

        void init();

        /**
         * @brief Update the sensor
         *
         * Read out sensor data and update register values in shared memory
         */
        void update();

        /**
         * @brief Stop the sensor
         *
         * Disable sensor 3V3 therefore disabling the sensor
         */
        void stop();

        /**
         * @brief Get the Json object - returns sensor data as json string
         *
         * @return std::string
         */
        std::string getJson() override;
    };

} // namespace Xerxes

#endif // __LIS2XY_HPP