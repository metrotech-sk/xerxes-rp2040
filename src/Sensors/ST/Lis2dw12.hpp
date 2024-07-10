#ifndef __LIS2DW12_HPP
#define __LIS2DW12_HPP

#include "Sensors/Sensor.hpp"

namespace Xerxes
{

    namespace CTRL1
    {
        // Enum for ODR (Output Data Rate)
        enum class ODR : uint8_t
        {
            PWR_DOWN = 0x00,       // 0000
            ODR_12_6_1_6HZ = 0x01, // 0001 12.5Hz in High Performance mode, 1.6Hz in Low Power mode
            ODR_12_5HZ = 0x02,     // 0010
            ODR_25HZ = 0x03,       // 0011
            ODR_50HZ = 0x04,       // 0100
            ODR_100HZ = 0x05,      // 0101
            ODR_200HZ = 0x06,      // 0110
            ODR_400_200HZ = 0x07,  // 0111 400Hz in High Performance mode, 200Hz in Low Power mode
            ODR_800_200HZ = 0x08,  // 1000 800Hz in High Performance mode, 200Hz in Low Power mode
            ODR_1600_200HZ = 0x09, // 1001 1600Hz in High Performance mode, 200Hz in Low Power mode
        };

        // Enum for MODE
        enum class MODE : uint8_t
        {
            LOW_PWR = 0x00,   // 00 Low Power Mode (12/14-bit resolution)
            HIGH_PERF = 0x01, // 01 High Performance Mode (14-bit resolution)
            ON_DEMAND = 0x02, // 10 On Demand Mode - single measurement
            // 0x03 reserved
        };

        // Enum for LP_MODE (Low Power Mode)
        enum class LP_MODE : uint8_t
        {
            LP_MODE1 = 0x00, // 00 Low Power Mode 1 /12-bit resolution
            LP_MODE2 = 0x01, // 01 Low Power Mode 2 /14-bit resolution
            LP_MODE3 = 0x02, // 10 Low Power Mode 3 /14-bit resolution
            LP_MODE4 = 0x03  // 11 Low Power Mode 4 /14-bit resolution
        };

        // Function to combine enums into CTRL1 (uint8_t)
        uint8_t val(ODR odr, MODE mode, LP_MODE lp_mode);

    } // namespace CTRL1

    namespace CTRL2
    {
        constexpr uint8_t BOOT = 0x40;        // Boot memory content (0: disabled; 1: enabled), this bit is self-cleared when the BOOT is completed
        constexpr uint8_t SOFT_RESET = 0x20;  // Soft reset acts as reset for all control registers, then goes to 0.
        constexpr uint8_t CS_PU_DISC = 0x10;  // Disconnect CS pull-up, 0: pull-up connected, 1: pull-up disconnected
        constexpr uint8_t BDU = 0x08;         // Block Data Update, 0: continuous update, 1: output registers not updated until MSB and LSB reading
        constexpr uint8_t IF_ADD_INC = 0x04;  // Register address automatically incremented during a multiple byte access with a serial interface (I2C or SPI) 0: disabled, 1: enabled
        constexpr uint8_t I2C_DISABLE = 0x02; // Disable I2C interface, 0: I2C enabled, 1: I2C disabled
        constexpr uint8_t SIM = 0x01;         // SPI Serial Interface Mode selection, 0: 4-wire interface, 1: 3-wire interface, default is 0: 4-wire interface

        typedef struct // TODO: verify order with datasheet
        {
            uint8_t SIM : 1;         // SPI Serial Interface Mode selection, 0: 4-wire interface, 1: 3-wire interface, default is 0: 4-wire interface
            uint8_t I2C_DISABLE : 1; // Disable I2C interface, 0: I2C enabled, 1: I2C disabled
            uint8_t IF_ADD_INC : 1;  // Register address automatically incremented during a multiple byte access with a serial interface (I2C or SPI) 0: disabled, 1: enabled
            uint8_t BDU : 1;         // Block Data Update, 0: continuous update, 1: output registers not updated until MSB and LSB reading
            uint8_t CS_PU_DISC : 1;  // Disconnect CS pull-up, 0: pull-up connected, 1: pull-up disconnected
            uint8_t SOFT_RESET : 1;  // Soft reset acts as reset for all control registers, then goes to 0.
            uint8_t BOOT : 1;        // Boot memory content (0: disabled; 1: enabled), this bit is self-cleared when the BOOT is completed
        } CTRL2_t;
    } // namespace CTRL2

    namespace CTRL6
    {
        constexpr uint8_t BW_FILT_ODR_2 = 0x00 << 4; // Bandwidth selection, 00: ODR/2, 01: ODR/4, 10: ODR/10, 11: ODR/20
        constexpr uint8_t BW_FILT_ODR_4 = 0x01 << 4;
        constexpr uint8_t BW_FILT_ODR_10 = 0x02 << 4;
        constexpr uint8_t BW_FILT_ODR_20 = 0x03 << 4;

        constexpr uint8_t FS_2G = 0x00 << 2;
        constexpr uint8_t FS_4G = 0x01 << 2;
        constexpr uint8_t FS_8G = 0x02 << 2;
        constexpr uint8_t FS_16G = 0x03 << 2;

        constexpr uint8_t FDS_LOW_PASS = 0x00 << 1;
        constexpr uint8_t FDS_HIGH_PASS = 0x01 << 1;

        constexpr uint8_t LOW_NOISE_DISABLED = 0x00;
        constexpr uint8_t LOW_NOISE_ENABLED = 0x01;

    } // namespace CTRL6

    typedef union
    {
        struct // LSB FIRST
        {
            uint8_t DRDY : 1;        // Data-ready status, 0: no data is available, 1: a new set of data is available
            uint8_t FF_IA : 1;       // Free-fall event status, 0: no free-fall event detected, 1: free-fall event detected
            uint8_t D6D_IA : 1;      // 6D event detection status, 0: no event detected, 1: change in position detected
            uint8_t SINGLE_TAP : 1;  // Single-tap event status, 0: no tap event detected, 1: tap event detected
            uint8_t DOUBLE_TAP : 1;  // Double-tap event status, 0: no tap event detected, 1: tap event detected
            uint8_t SLEEP_STATE : 1; // Sleep event status, 0: no sleep event detected, 1: sleep event detected
            uint8_t WU_IA : 1;       // Wake-up event detection status, 0: no wake-up event detected, 1: wake-up event detected
            uint8_t FIFO_THS : 1;    // FIFO threshold status flag, 0: FIFO filling is lower than the threshold, 1: FIFO filling is equal to or higher than the threshold
        } bits;
        uint8_t reg;
    } Status_t;

    namespace REG
    {
        // register addresses
        static constexpr uint8_t OUT_T_L = 0x0D;
        static constexpr uint8_t OUT_T_H = 0x0E;
        static constexpr uint8_t WHO_AM_I = 0x0F;
        // RESERVED 0x10 - 0x1F
        static constexpr uint8_t CTRL1 = 0x20;
        static constexpr uint8_t CTRL2 = 0x21;
        static constexpr uint8_t CTRL3 = 0x22;
        static constexpr uint8_t CTRL4 = 0x23;
        static constexpr uint8_t CTRL5 = 0x24;
        static constexpr uint8_t CTRL6 = 0x25;
        static constexpr uint8_t OUT_T = 0x26;
        static constexpr uint8_t STATUS = 0x27;
        static constexpr uint8_t OUT_X_L = 0x28;
        static constexpr uint8_t OUT_X_H = 0x29;
        static constexpr uint8_t OUT_Y_L = 0x2A;
        static constexpr uint8_t OUT_Y_H = 0x2B;
        static constexpr uint8_t OUT_Z_L = 0x2C;
        static constexpr uint8_t OUT_Z_H = 0x2D;
        static constexpr uint8_t FIFO_CTRL = 0x2E;
        static constexpr uint8_t FIFO_SAMPLES = 0x2F;
    }

    static constexpr uint8_t READ = 0x80;
    static constexpr uint8_t WRITE = 0x00;

    class LIS2 : public Sensor
    {
    private:
        // sensor specific values
        static constexpr uint8_t WHO_AM_I_VAL = 0x44;

        /// @brief Read whoami register and return value - should be 0x44
        const uint8_t whoami() const;

        /// @brief Read register and return value
        const uint8_t readRegister(uint8_t reg) const;

        const bool writeRegister(uint8_t reg, uint8_t val) const;

        const bool dataReady() const;

        constexpr static size_t N_SAMPLES = 1024;
        constexpr static uint16_t FREQ = 200;

    protected:
        // typedef Sensor as super class for easier access
        typedef Sensor super;

    public:
        using Sensor::Sensor;

        /**
         * @brief Init the sensor
         *
         * Activate 3V3 power supply for the sensor,
         * initialize SPI communication and read out first sequence
         */
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

#endif // __LIS2DW12_HPP