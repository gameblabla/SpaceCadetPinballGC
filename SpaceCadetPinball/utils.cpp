#include "utils.h"

int32_t utils::swap_i32(int32_t value)
{
    return ((value & 0x000000ff) << 24) | ((value & 0x0000ff00) << 8) | ((value & 0x00ff0000) >> 8) | ((value & 0xff000000) >> 24);
}

uint32_t utils::swap_u32(uint32_t value)
{
    return ((value & 0x000000ff) << 24) | ((value & 0x0000ff00) << 8) | ((value & 0x00ff0000) >> 8) | ((value & 0xff000000) >> 24);
}

int16_t utils::swap_i16(int16_t value)
{
    return ((value & 0x00ff) << 8) | ((value & 0xff00) >> 8);
}

uint16_t utils::swap_u16(uint16_t value)
{
    return ((value & 0x00ff) << 8) | ((value & 0xff00) >> 8);
}