#include "wii_graphics.h"

#include <cstdio>
#include <cstring>
#include <malloc.h>

// #include "ogc/conf.h"

void *wii_graphics::gpFifo = nullptr;
void *wii_graphics::frameBuffer[2] = {nullptr, nullptr};
GXRModeObj *wii_graphics::rmode = nullptr;
uint8_t wii_graphics::currentFramebuffer = 0;
bool wii_graphics::isConsoleInitialized = false;

void wii_graphics::Initialize()
{
    // Init the vi

    VIDEO_Init();

    // Get video mode and allocate 2 framebuffers

    rmode = VIDEO_GetPreferredMode(NULL);
    printf("Framebuffer: %ux%u\n", rmode->fbWidth, rmode->efbHeight);
    printf("VI: %ux%u\n", rmode->viWidth, rmode->viHeight);
    printf("TV Mode: %u\n", rmode->viTVMode);
    printf("XFB Mode: %u\n", rmode->xfbMode);
    printf("XFB Height: %u\n", rmode->xfbHeight);
    printf("Antialiasing: %u\n", rmode->aa);
    printf("Field rendering: %u\n\n", rmode->field_rendering);

    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    currentFramebuffer = 0;

    // if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
    // {
    //     rmode->viWidth = 600;
    // }
    // else
    // {
    //     rmode->viWidth = 600;
    // }

    // if (rmode == &TVPal576IntDfScale || rmode == &TVPal576ProgScale)
    // {
    //     rmode->viXOrigin = (VI_MAX_WIDTH_PAL - rmode->viWidth) / 2;
    //     rmode->viYOrigin = (VI_MAX_HEIGHT_PAL - rmode->viHeight) / 2;
    // }
    // else
    // {
    //     rmode->viXOrigin = (VI_MAX_WIDTH_NTSC - rmode->viWidth) / 2;
    //     rmode->viYOrigin = (VI_MAX_HEIGHT_NTSC - rmode->viHeight) / 2;
    // }

    rmode->viWidth = 640;
    rmode->viHeight = 448;
    rmode->fbWidth = 640;
    rmode->efbHeight = 448;
    rmode->xfbHeight = 448;
    rmode->viXOrigin = (VI_MAX_WIDTH_NTSC - rmode->viWidth) >> 1;
    rmode->viYOrigin = (VI_MAX_HEIGHT_NTSC - rmode->viHeight) >> 1;

    // Set some video properties

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[currentFramebuffer]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    // Setup the fifo and then init the flipper

    gpFifo = memalign(32, FIFO_SIZE);
    memset(gpFifo, 0, FIFO_SIZE);

    GX_Init(gpFifo, FIFO_SIZE);

    // Set some initial graphics state

    GXColor clearColor = {0, 0, 0, 255};
    GX_SetCopyClear(clearColor, GX_MAX_Z24);

    float yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
    uint32_t xfbHeight = GX_SetDispCopyYScale(yscale);

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

    // if (rmode->aa)
    //     GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    // else
    //     GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

    GX_SetCullMode(GX_CULL_BACK);
    GX_CopyDisp(frameBuffer[currentFramebuffer], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);

    // Texture and TEV configuration

    GX_SetNumChans(1);
    GX_SetNumTexGens(1);

    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    //GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    //GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
}

void wii_graphics::LoadOrthoProjectionMatrix(float top, float bottom, float left, float right, float near, float far)
{
    Mtx44 projectionMatrix;
    guOrtho(projectionMatrix, top, bottom, left, right, near, far);
    GX_LoadProjectionMtx(projectionMatrix, GX_ORTHOGRAPHIC);
}

void wii_graphics::Load2DModelViewMatrix(uint32_t matrixIndex, float x, float y)
{
    Mtx modelMatrix;
    guMtxIdentity(modelMatrix);
    guMtxTransApply(modelMatrix, modelMatrix, x, y, -5.0f);

    Mtx viewMatrix;
    guVector cam = {0.0F, 0.0F, 0.0F};
    guVector up = {0.0F, 1.0F, 0.0F};
    guVector look = {0.0F, 0.0F, -1.0F};
    guLookAt(viewMatrix, &cam, &up, &look);

    Mtx modelViewMatrix;
    guMtxConcat(viewMatrix, modelMatrix, modelViewMatrix);
    GX_LoadPosMtxImm(modelViewMatrix, matrixIndex);

    GX_SetCurrentMtx(matrixIndex);
}

uint32_t wii_graphics::Create2DQuadDisplayList(void *displayList, float top, float bottom, float left, float right, float uvTop, float uvBottom, float uvLeft, float uvRight)
{
    // Configure vertex formats

    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    //GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    //GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGB8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    memset(displayList, 0, MAX_DISPLAY_LIST_SIZE);

    // Invalidate vertex cache

    GX_InvVtxCache();

    // It's necessary to flush the data cache of the display list
    // just before filling it.

    DCInvalidateRange(displayList, MAX_DISPLAY_LIST_SIZE);

    // Start generating the display list

    GX_BeginDispList(displayList, MAX_DISPLAY_LIST_SIZE);

    GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
    GX_Position3f32(left, top, 0.0f);
    //GX_Color3f32(1.0f, 0.0f, 0.0f);
    GX_TexCoord2f32(uvLeft, uvTop);

    GX_Position3f32(right, top, 0.0f);
    //GX_Color3f32(0.0f, 1.0f, 0.0f);
    GX_TexCoord2f32(uvRight, uvTop);

    GX_Position3f32(left, bottom, 0.0f);
    //GX_Color3f32(0.0f, 0.0f, 1.0f);
    GX_TexCoord2f32(uvLeft, uvBottom);

    GX_Position3f32(right, bottom, 0.0f);
    //GX_Color3f32(1.0f, 1.0f, 1.0f);
    GX_TexCoord2f32(uvRight, uvBottom);

    GX_End();

    return GX_EndDispList();
}

void wii_graphics::CallDisplayList(void *displayList, uint32_t displayListSize)
{
    GX_CallDispList(displayList, displayListSize);
}

void wii_graphics::CreateTextureObject(GXTexObj *textureObject, uint8_t *textureData, uint16_t width, uint16_t height, uint32_t format, uint8_t wrap, uint8_t filter)
{
    GX_InvalidateTexAll();

    GX_InitTexObj(textureObject, textureData, width, height, format, wrap, wrap, GX_FALSE);
    GX_InitTexObjFilterMode(textureObject, filter, filter);
}

void wii_graphics::LoadTextureObject(GXTexObj *textureObject, uint8_t mapIndex)
{
    GX_LoadTexObj(textureObject, mapIndex);
}

uint32_t wii_graphics::GetTextureSize(uint16_t width, uint16_t height, uint32_t format, uint8_t mipmap, uint8_t maxlod)
{
    return GX_GetTexBufferSize(width, height, format, mipmap, maxlod);
}

void wii_graphics::FlushDataCache(void *startAddress, uint32_t size)
{
    DCFlushRange(startAddress, size);
}

void wii_graphics::SwapBuffers()
{
    GX_CopyDisp(frameBuffer[currentFramebuffer], GX_TRUE);
    GX_DrawDone();
    SetNextFramebuffer(currentFramebuffer);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    currentFramebuffer ^= 1;
}

void wii_graphics::SetNextFramebuffer(uint8_t framebufferIndex)
{
    VIDEO_SetNextFramebuffer(frameBuffer[framebufferIndex]);
}

void wii_graphics::InitializeConsole()
{
    if (isConsoleInitialized)
        return;

    //console_init(frameBuffer[0], 200, 200, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    SYS_ConsoleInit(rmode, 32, 32, rmode->fbWidth, rmode->xfbHeight);
    isConsoleInitialized = true;
}