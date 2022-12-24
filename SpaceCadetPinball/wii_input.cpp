#include "wii_input.h"

void wii_input::Initialize()
{
    PAD_Init();
#ifndef HW_DOL
    WPAD_Init();
#endif
}

void wii_input::ScanPads()
{
    PAD_ScanPads();
#ifndef HW_DOL
    WPAD_ScanPads();
#endif
}

uint32_t wii_input::GetWiiButtonsDown(int32_t padIndex)
{
	#ifndef HW_DOL
    return WPAD_ButtonsDown(padIndex);
    #else
    return 0;
    #endif
}

uint32_t wii_input::GetGCButtonsDown(int32_t padIndex)
{
    return PAD_ButtonsDown(padIndex);
}

uint32_t wii_input::GetWiiButtonsUp(int32_t padIndex)
{
	#ifndef HW_DOL
    return WPAD_ButtonsUp(padIndex);
    #else
    return 0;
    #endif
}

uint32_t wii_input::GetGCButtonsUp(int32_t padIndex)
{
    return PAD_ButtonsUp(padIndex);
}

uint32_t wii_input::GetWiiButtonsHeld(int32_t padIndex)
{
	#ifndef HW_DOL
    return WPAD_ButtonsHeld(padIndex);
    #else
    return 0;
    #endif
}

uint32_t wii_input::GetGCButtonsHeld(int32_t padIndex)
{
    return PAD_ButtonsHeld(padIndex);
}
