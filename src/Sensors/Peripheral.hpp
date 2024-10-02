#ifndef __PERIPHERAL_HPP
#define __PERIPHERAL_HPP

#include <cstdint>
#include <DeviceIds.h>
#include <ostream>
#include <string>
#include "Hardware/Board/xerxes_rp2040.h"

namespace Xerxes
{

    /**
     * @brief Check if the data received from the spi bus is valid
     *
     * @param data array of data bytes
     * @param len length of the data array
     * @return true if data is valid
     * @return false if data is invalid - all bits set to 1 or 0
     */
    bool isSpiDataOk(uint8_t *data, uint8_t len);

    /**
     * @brief Peripheral base class with pure virtual functions - init, update, stop
     *
     */
    class Peripheral
    {
    private:
    protected:
        devid_t _devid{0};

        constexpr static uint32_t _usInS = 1000000; // microseconds in a second
        std::string _label{"Xerxes Peripheral"};

    public:
        Peripheral();
        ~Peripheral();

        // pure virtual functions
        virtual void init() = 0;
        virtual void update() = 0;
        virtual void stop() = 0;

        /**
         * @brief Get the Devid object
         *
         * @return devid_t device id of the peripheral - based on the device type
         */
        devid_t getDevid();

        virtual std::string getJson() = 0;

        virtual std::string getInfoJson() const = 0;
    };

} // namespace Xerxes

#endif // !__PERIPHERAL_HPP