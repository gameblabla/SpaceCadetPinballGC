#include "wii_input.h"

void wii_input::Initialize()
{
    PAD_Init();
    WPAD_Init();
}

void wii_input::ScanPads()
{
    PAD_ScanPads();
    WPAD_ScanPads();
}

uint32_t wii_input::GetWiiButtonsDown(int32_t padIndex)
{
    return WPAD_ButtonsDown(padIndex);
}

uint32_t wii_input::GetGCButtonsDown(int32_t padIndex)
{
    return PAD_ButtonsDown(padIndex);
}

uint32_t wii_input::GetWiiButtonsUp(int32_t padIndex)
{
    return WPAD_ButtonsUp(padIndex);
}

uint32_t wii_input::GetGCButtonsUp(int32_t padIndex)
{
    return PAD_ButtonsUp(padIndex);
}

uint32_t wii_input::GetWiiButtonsHeld(int32_t padIndex)
{
    return WPAD_ButtonsHeld(padIndex);
}

uint32_t wii_input::GetGCButtonsHeld(int32_t padIndex)
{
    return PAD_ButtonsHeld(padIndex);
}