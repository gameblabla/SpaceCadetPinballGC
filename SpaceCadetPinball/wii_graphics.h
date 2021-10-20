#pragma once

#include <gccore.h>

constexpr uint32_t FIFO_SIZE = 64 * 1024;
constexpr uint32_t MAX_DISPLAY_LIST_SIZE = 128;

class wii_graphics
{
private:
    static void *gpFifo;
    static void *frameBuffer[2];
    static GXRModeObj *rmode;
    static uint32_t currentFramebuffer;

public:
    static void Initialize();
    static void LoadOrthoProjectionMatrix(float top, float bottom, float left, float right, float near, float far);
    static void Load2DModelViewMatrix(uint32_t matrixIndex, float x, float y);
    static uint32_t Create2DQuadDisplayList(void *displayList, float top, float bottom, float left, float right);
    static void CallDisplayList(void *displayList, uint32_t displayListSize);
    static void CreateTextureObject(GXTexObj *textureObject, uint8_t *textureData, uint16_t width, uint16_t height, uint32_t format, uint8_t wrap, uint8_t filter);
    static void UpdateTextureObjectData(GXTexObj *textureObject, uint8_t *textureData);
    static void LoadTextureObject(GXTexObj *textureObject, uint8_t mapIndex);
    static uint32_t GetTextureSize(uint16_t width, uint16_t height, uint32_t format, uint8_t mipmap, uint8_t maxlod);
    static void SwapBuffers();
};