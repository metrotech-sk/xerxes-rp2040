#include "UserFlash.hpp"

#include "hardware/flash.h"
#include "hardware/sync.h"
#include <cstring>
#include "MemoryMap.h"
#include "Core/Definitions.h"

bool userInitFlash(uint8_t *memTable)
{
    // disable interrupts first
    auto status = save_and_disable_interrupts();

    const uint8_t *flash_target_contents = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET); // Flash memory contents
    uint64_t *uid = (uint64_t *)(memTable + UID_OFFSET);                                      // Unique ID of the device

    // read UID
    flash_get_unique_id((uint8_t *)uid);

    // read flash
    std::memcpy((uint8_t *)memTable, flash_target_contents, VOLATILE_OFFSET);

    // check if flash is empty
    bool empty = true;
    for (uint16_t i = 0; i < VOLATILE_OFFSET; i++)
    {
        if (memTable[i] != 0xFF)
        {
            empty = false;
            break;
        }
    }

    restore_interrupts(status);
    if (empty)
        return false;
    else
        return true;
}

void updateFlash(const uint8_t *memTable)
{
    // disable interrupts first
    auto status = save_and_disable_interrupts();

    // erase flash, must be done in sector size, 4KB, it takes 49ms
    flash_range_erase(FLASH_TARGET_OFFSET, VOLATILE_OFFSET);

    // copy memory to buffer
    uint8_t memImage[VOLATILE_OFFSET];
    std::memcpy(memImage, (uint8_t *)memTable, sizeof(memImage));

    // write flash, must be done in page size (256bytes), approx 400us
    flash_range_program(FLASH_TARGET_OFFSET, memImage, VOLATILE_OFFSET);

    // finally, restore interrupts
    restore_interrupts(status);
}