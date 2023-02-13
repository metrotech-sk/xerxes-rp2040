#ifndef __REGISTER_HPP
#define __REGISTER_HPP


#include "Core/Definitions.h"


namespace Xerxes
{
    

class Register
{
private:
    /* data */
public:
    Register(/* args */);
    ~Register();

    uint8_t memTable[REGISTER_SIZE];

    float* gainPv0       = (float *)(memTable + GAIN_PV0_OFFSET);
    float* gainPv1       = (float *)(memTable + GAIN_PV1_OFFSET);
    float* gainPv2       = (float *)(memTable + GAIN_PV2_OFFSET);
    float* gainPv3       = (float *)(memTable + GAIN_PV3_OFFSET);


    float* offsetPv0     = (float *)(memTable + OFFSET_PV0_OFFSET);
    float* offsetPv1     = (float *)(memTable + OFFSET_PV1_OFFSET);
    float* offsetPv2     = (float *)(memTable + OFFSET_PV2_OFFSET);
    float* offsetPv3     = (float *)(memTable + OFFSET_PV3_OFFSET);

    uint32_t *desiredCycleTimeUs     = (uint32_t *)(memTable + OFFSET_DESIRED_CYCLE_TIME);  ///< Desired cycle time of sensor loop in microseconds
    uint8_t *devAddress              = (uint8_t *)(memTable + OFFSET_ADDRESS);  ///< Address of the device (1 byte)
    ConfigBitsUnion *config          = (ConfigBitsUnion *)(memTable + OFFSET_CONFIG_BITS);  ///< Config bits of the device (1 byte)
    uint32_t *netCycleTimeUs         = (uint32_t *)(memTable + OFFSET_NET_CYCLE_TIME);  ///< Actual cycle time of measurement loop in microseconds


    /* ### VOLATILE - PROCESS VALUES ### */
    float* pv0           = (float *)(memTable + PV0_OFFSET);    ///< Pointer to process value 0
    float* pv1           = (float *)(memTable + PV1_OFFSET);    ///< Pointer to process value 1
    float* pv2           = (float *)(memTable + PV2_OFFSET);    ///< Pointer to process value 2
    float* pv3           = (float *)(memTable + PV3_OFFSET);    ///< Pointer to process value 3

    float* meanPv0       = (float *)(memTable + MEAN_PV0_OFFSET);
    float* meanPv1       = (float *)(memTable + MEAN_PV1_OFFSET);
    float* meanPv2       = (float *)(memTable + MEAN_PV2_OFFSET);
    float* meanPv3       = (float *)(memTable + MEAN_PV3_OFFSET);

    float* stdDevPv0     = (float *)(memTable + STDDEV_PV0_OFFSET);
    float* stdDevPv1     = (float *)(memTable + STDDEV_PV1_OFFSET);
    float* stdDevPv2     = (float *)(memTable + STDDEV_PV2_OFFSET);
    float* stdDevPv3     = (float *)(memTable + STDDEV_PV3_OFFSET);

    float* minPv0        = (float *)(memTable + MIN_PV0_OFFSET);
    float* minPv1        = (float *)(memTable + MIN_PV1_OFFSET);
    float* minPv2        = (float *)(memTable + MIN_PV2_OFFSET);
    float* minPv3        = (float *)(memTable + MIN_PV3_OFFSET);

    float* maxPv0        = (float *)(memTable + MAX_PV0_OFFSET);
    float* maxPv1        = (float *)(memTable + MAX_PV1_OFFSET);
    float* maxPv2        = (float *)(memTable + MAX_PV2_OFFSET);
    float* maxPv3        = (float *)(memTable + MAX_PV3_OFFSET);

    uint32_t* dv0       = (uint32_t *)(memTable + DV0_OFFSET);  ///< Pointer to digital value 0
    uint32_t* dv1       = (uint32_t *)(memTable + DV1_OFFSET);  ///< Pointer to digital value 1
    uint32_t* dv2       = (uint32_t *)(memTable + DV2_OFFSET);  ///< Pointer to digital value 2
    uint32_t* dv3       = (uint32_t *)(memTable + DV3_OFFSET);  ///< Pointer to digital value 3

    /* ### READ ONLY VALUES ### */
    /** @brief 0x55AA55AA = unlocked, anything else = locked */
    uint32_t* memUnlocked   = (uint32_t *)(memTable + MEM_UNLOCKED_OFFSET);

    uint64_t* error      = (uint64_t *)(memTable + ERROR_OFFSET);   ///< Error register, holds error codes
    uint64_t* status     = (uint64_t *)(memTable + STATUS_OFFSET);  ///< Status register, holds status codes
    uint64_t* uid        = (uint64_t *)(memTable + UID_OFFSET);     ///< Unique ID of the device

    /* ### MESSAGE STRING MEMORY ### */
    char* message    = (char *)(memTable + MESSAGE_OFFSET); ///< Message string, holds messages (debug, info, warning, error)
};


} // namespace Xerxes


#endif // !__REGISTER_HPP