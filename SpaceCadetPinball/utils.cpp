#include "utils.h"

int32_t utils::swap_i32(const int32_t value)
{
#ifdef BIG_ENDIAN
    return ((value & 0x000000ff) << 24) | ((value & 0x0000ff00) << 8) | ((value & 0x00ff0000) >> 8) | ((value & 0xff000000) >> 24);
#else
    return value;
#endif
}

uint32_t utils::swap_u32(const uint32_t value)
{
#ifdef BIG_ENDIAN
    return ((value & 0x000000ff) << 24) | ((value & 0x0000ff00) << 8) | ((value & 0x00ff0000) >> 8) | ((value & 0xff000000) >> 24);
#else
    return value;
#endif
}

int16_t utils::swap_i16(const int16_t value)
{
#ifdef BIG_ENDIAN
    return ((value & 0x00ff) << 8) | ((value & 0xff00) >> 8);
#else
    return value;
#endif
}

uint16_t utils::swap_u16(const uint16_t value)
{
#ifdef BIG_ENDIAN
    return ((value & 0x00ff) << 8) | ((value & 0xff00) >> 8);
#else
    return value;
#endif
}

float utils::swap_float(const float value)
{
#ifdef BIG_ENDIAN
    float retVal;
    char *floatToConvert = (char *)&value;
    char *returnFloat = (char *)&retVal;

    returnFloat[0] = floatToConvert[3];
    returnFloat[1] = floatToConvert[2];
    returnFloat[2] = floatToConvert[1];
    returnFloat[3] = floatToConvert[0];

    return retVal;
#else
    return value;
#endif
}

uint16_t utils::align(const uint16_t value, const uint16_t alignment)
{
    uint p = value % alignment;
    return p == 0 ? value : value + (alignment - p);
}