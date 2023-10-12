#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include "AnalogInput.hpp"

namespace Xerxes
{

//  REGISTERS
#define ADS1X15_REG_CONVERT         0x00
#define ADS1X15_REG_CONFIG          0x01
#define ADS1X15_REG_LOW_THRESHOLD   0x02
#define ADS1X15_REG_HIGH_THRESHOLD  0x03


//  CONFIG REGISTER

//  BIT 15      Operational Status           // 1 << 15
#define ADS1X15_OS_BUSY             0x0000
#define ADS1X15_OS_NOT_BUSY         0x8000
#define ADS1X15_OS_START_SINGLE     0x8000

//  BIT 12-14   read differential
#define ADS1X15_MUX_DIFF_0_1        0x0000
#define ADS1X15_MUX_DIFF_0_3        0x1000
#define ADS1X15_MUX_DIFF_1_3        0x2000
#define ADS1X15_MUX_DIFF_2_3        0x3000
//              read single
#define ADS1X15_READ_0              0x4000   //  pin << 12
#define ADS1X15_READ_1              0x5000   //  pin = 0..3
#define ADS1X15_READ_2              0x6000
#define ADS1X15_READ_3              0x7000


//  BIT 9-11    gain                         //  (0..5) << 9
#define ADS1X15_PGA_6_144V          0x0000   //  voltage
#define ADS1X15_PGA_4_096V          0x0200   //
#define ADS1X15_PGA_2_048V          0x0400   //  default
#define ADS1X15_PGA_1_024V          0x0600
#define ADS1X15_PGA_0_512V          0x0800
#define ADS1X15_PGA_0_256V          0x0A00

//  BIT 8       mode                         //  1 << 8
#define ADS1X15_MODE_CONTINUE       0x0000
#define ADS1X15_MODE_SINGLE         0x0100

//  BIT 5-7     data rate                    //  (0..7) << 5
#define ADS1X15_DR_8SPS             0x0000
#define ADS1X15_DR_16SPS            0x0020
#define ADS1X15_DR_32SPS            0x0040
#define ADS1X15_DR_64SPS            0x0060
#define ADS1X15_DR_128SPS           0x0080   //  default
#define ADS1X15_DR_250SPS           0x00A0
#define ADS1X15_DR_475SPS           0x00C0
#define ADS1X15_DR_860SPS           0x00E0

//  BIT 4 comparator modi                    // 1 << 4
#define ADS1X15_COMP_MODE_TRADITIONAL   0x0000
#define ADS1X15_COMP_MODE_WINDOW        0x0010

//  BIT 3 ALERT active value                 // 1 << 3
#define ADS1X15_COMP_POL_ACTIV_LOW      0x0000
#define ADS1X15_COMP_POL_ACTIV_HIGH     0x0008

//  BIT 2 ALERT latching                     // 1 << 2
#define ADS1X15_COMP_NON_LATCH          0x0000
#define ADS1X15_COMP_LATCH              0x0004

//  BIT 0-1 ALERT mode                       // (0..3)
#define ADS1X15_COMP_QUE_1_CONV         0x0000  //  trigger alert after 1 convert
#define ADS1X15_COMP_QUE_2_CONV         0x0001  //  trigger alert after 2 converts
#define ADS1X15_COMP_QUE_4_CONV         0x0002  //  trigger alert after 4 converts
#define ADS1X15_COMP_QUE_NONE           0x0003  //  disable comparator


// _CONFIG masks
//
//  |  bit  |  description           |
//  |:-----:|:-----------------------|
//  |   0   |  # channels            |
//  |   1   |  -                     |
//  |   2   |  resolution            |
//  |   3   |  -                     |
//  |   4   |  GAIN supported        |
//  |   5   |  COMPARATOR supported  |
//  |   6   |  -                     |
//  |   7   |  -                     |
//
#define ADS_CONF_CHAN_1  0x00
#define ADS_CONF_CHAN_4  0x01
#define ADS_CONF_RES_12  0x00
#define ADS_CONF_RES_16  0x04
#define ADS_CONF_NOGAIN  0x00
#define ADS_CONF_GAIN    0x10
#define ADS_CONF_NOCOMP  0x00
#define ADS_CONF_COMP    0x20


constexpr float VREF = 2.5;  // reference voltage

class DiscreteAnalog : public AnalogInput {
private:
    void _WriteConfig(const uint16_t config);
    void _SetContinuous();
    int16_t _ReadConversionResult();
    int16_t _ReadCh0();
public:
    using AnalogInput::AnalogInput;
    using AnalogInput::operator=;
    using AnalogInput::getJson;
    using AnalogInput::getJsonLast;
    using AnalogInput::getJsonMin;
    using AnalogInput::getJsonMax;
    using AnalogInput::getJsonMean;
    using AnalogInput::getJsonStdDev;

    ~DiscreteAnalog();

    void init();

    void update();

    void stop();

};
    
}  // namespace Xerxes