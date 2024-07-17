#include "Various.hpp"
#include <malloc.h>

uint32_t getTotalHeap(void)
{
    extern char __StackLimit, __bss_end__;

    return &__StackLimit - &__bss_end__;
}

uint32_t getFreeHeap(void)
{
    struct mallinfo m = mallinfo();

    return getTotalHeap() - m.uordblks;
}

uint32_t getUsedHeap(void)
{
    struct mallinfo m = mallinfo();

    return m.uordblks;
}