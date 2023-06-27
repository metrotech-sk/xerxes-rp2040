#ifndef __DIGITALINPUTOUTPUT_HPP
#define __DIGITALINPUTOUTPUT_HPP

#include "Sensors/Peripheral.hpp"
#include "Core/Register.hpp"
#include "Hardware/Board/xerxes_rp2040.h"
#include <string>


namespace Xerxes
{


class DigitalInputOutput : public Peripheral
{
protected:
    Register* _reg;

    uint32_t used_iomask = SHIELD_MASK;

    constexpr static uint32_t _updateRateHz = 10000;  // update frequency in Hz
    constexpr static uint32_t _updateRateUs = _usInS / _updateRateHz;  // update rate in microseconds

public:
    /**
     * @brief Construct a new Digital Input Output object
     * 
     * @param reg - pointer to the register to store the digital values and other data
     */
    DigitalInputOutput(Register *reg) : 
        _reg(reg)
    {};

    /**
     * @brief Construct a new Digital Input Output object
     * 
     * @note This constructor should not be used, it is only here to allow the creation of an array of DigitalInputOutput objects
     */
    DigitalInputOutput() : 
        _reg(nullptr)
    {};
    
    /**
     * @brief Destroy the Digital Input Output object
     */
    ~DigitalInputOutput();

    /**
     * @brief Initialize the DIO
     * 
     * @param iomask - 32 bit mask of the DIO pins, 0 = do not change direction, 1 = change direction
     * @param direction - 32 bit mask of the DIO pins, 0 = input, 1 = output
     * 
     */
    void init(uint32_t iomask, uint32_t direction);


    /**
     * @brief Update the DIO
     * 
     */
    void update();


    /**
     * @brief Stop the DIO
     * 
     */
    void stop();


    /**
     * @brief Get the Json object - returns sensor data as json string
     * 
     * @return std::string
     */
    std::string getJson();
};


}   // namespace Xerxes


#endif // !__DIGITALINPUTOUTPUT_HPP