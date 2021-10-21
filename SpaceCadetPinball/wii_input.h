#pragma once

#include <gccore.h>
#include <wiiuse/wpad.h>

class wii_input
{
public:
    static void Initialize();
    static void ScanPads();

    static uint32_t GetWiiButtonsDown(int32_t padIndex);
    static uint32_t GetGCButtonsDown(int32_t padIndex);
    static uint32_t GetWiiButtonsUp(int32_t padIndex);
    static uint32_t GetGCButtonsUp(int32_t padIndex);
    static uint32_t GetWiiButtonsHeld(int32_t padIndex);
    static uint32_t GetGCButtonsHeld(int32_t padIndex);
};