#include "pch.h"
#include "winmain.h"

#include <malloc.h>

#include "control.h"
#include "midi.h"
#include "options.h"
#include "pb.h"
#include "pinball.h"
#include "render.h"
#include "Sound.h"
#include "utils.h"
#include "wii_graphics.h"
#include "wii_input.h"

int winmain::bQuit = 0;
int winmain::activated;
int winmain::DispFrameRate = 0;
int winmain::DispGRhistory = 0;
int winmain::single_step = 0;
int winmain::last_mouse_x;
int winmain::last_mouse_y;
int winmain::mouse_down;
int winmain::no_time_loss;

std::string winmain::DatFileName;
bool winmain::ShowSpriteViewer = false;
bool winmain::LaunchBallEnabled = true;
bool winmain::HighScoresEnabled = true;
bool winmain::DemoActive = false;
char *winmain::BasePath;
std::string winmain::FpsDetails;
double winmain::UpdateToFrameRatio;
winmain::DurationMs winmain::TargetFrameTime;
optionsStruct &winmain::Options = options::Options;

int winmain::WinMain(LPCSTR lpCmdLine)
{
	std::set_new_handler(memalloc_failure);

	// Initialize graphics and input

	wii_graphics::Initialize();
	wii_input::Initialize();

	// Set the base path for PINBALL.DAT
#ifdef HW_DOL
	BasePath = (char *)"/apps/SpaceCadetPinball/Data/";
#else
	BasePath = (char *)"sd:/apps/SpaceCadetPinball/Data/";
#endif
	pinball::quickFlag = 0; // strstr(lpCmdLine, "-quick") != nullptr;
	DatFileName = options::get_string("Pinball Data", pinball::get_rc_string(168, 0));

	// Check for full tilt .dat file and switch to it automatically

	auto cadetFilePath = pinball::make_path_name("CADET.DAT");
	auto cadetDat = fopen(cadetFilePath.c_str(), "r");
	if (cadetDat)
	{
		fclose(cadetDat);
		DatFileName = "CADET.DAT";
		pb::FullTiltMode = true;
	}

	// PB init from message handler

	{
		options::init();
		if (!Sound::Init(Options.SoundChannels, Options.Sounds))
			Options.Sounds = false;

		if (!pinball::quickFlag && !midi::music_init())
			Options.Music = false;

		if (pb::init())
		{
			PrintFatalError("Could not load game data: %s file is missing.\n", DatFileName.c_str());
		}
	}

	// Texture data and create texture object

	uint16_t texWidth = render::vscreen->Width;
	uint16_t texHeight = render::vscreen->Height;
	uint32_t textureSize = wii_graphics::GetTextureSize(texWidth, texHeight, GX_TF_RGBA8, 0, 0);
	GXTexObj textureObject;
	uint8_t *textureData = (uint8_t *)memalign(32, textureSize);
	memset(textureData, 0, textureSize);
	wii_graphics::CreateTextureObject(&textureObject, textureData, texWidth, texHeight, GX_TF_RGBA8, GX_CLAMP, GX_LINEAR);
	wii_graphics::LoadTextureObject(&textureObject, GX_TEXMAP0);

	// Load the projection matrix according to screen texture resolution

	wii_graphics::LoadOrthoProjectionMatrix(0, texHeight, 0, texWidth, 0.1f, 10.0f);

	// Create quad display list for the board and for the side bar

	// Normal values for the board's size are: 0, texHeight, 0, 375
	// but reduced the size a bit to compensate overscan in a TV.
	// Ideally this should be done adjusting the VI, but I was unsuccessful and had crashes.

	void *boardDisplayList = memalign(32, MAX_DISPLAY_LIST_SIZE);
	uint32_t boardDisplayListSize = wii_graphics::Create2DQuadDisplayList(boardDisplayList, 12, texHeight - 10, 16, 371, 0.0f, 1.0f, 0.0f, 0.625f);

	if (boardDisplayListSize == 0)
	{
		PrintFatalError("Board display list exceeded size.\n");
	}
	else
	{
		printf("Board display list size: %u\n", boardDisplayListSize);
	}

	void *sidebarDisplayList = memalign(32, MAX_DISPLAY_LIST_SIZE);
	uint32_t sidebarDisplayListSize = wii_graphics::Create2DQuadDisplayList(sidebarDisplayList, 0, texHeight, 375, texWidth, 0.0f, 1.0f, 0.625f, 1.0f);

	if (sidebarDisplayListSize == 0)
	{
		PrintFatalError("Sidebar display list exceeded size.\n");
	}
	else
	{
		printf("Sidebar display list size: %u\n", sidebarDisplayListSize);
	}

	// Initialize game

	pb::reset_table();
	pb::firsttime_setup();
	pb::replay_level(0);

	// Begin main loop

	bQuit = false;

	while (!bQuit)
	{
		// Input

		wii_input::ScanPads();

		uint32_t wiiButtonsDown = wii_input::GetWiiButtonsDown(0);
		uint32_t gcButtonsDown = wii_input::GetGCButtonsDown(0);
		uint32_t wiiButtonsUp = wii_input::GetWiiButtonsUp(0);
		uint32_t gcButtonsUp = wii_input::GetGCButtonsUp(0);

		if ((wiiButtonsDown & WPAD_BUTTON_HOME) ||
			(gcButtonsDown & PAD_TRIGGER_Z))
			break;

		if ((wiiButtonsDown & WPAD_BUTTON_PLUS) ||
			(gcButtonsDown & PAD_BUTTON_START))
			pause();

		if ((wiiButtonsDown & WPAD_BUTTON_1) ||
			(gcButtonsDown & PAD_BUTTON_Y))
			new_game();

		pb::keydown(wiiButtonsDown, gcButtonsDown);
		pb::keyup(wiiButtonsUp, gcButtonsUp);

		if (!single_step)
		{
			// Update game when not paused

			pb::frame(1000.0f / 60.0f);

			// Copy game screen buffer to texture

			GX_InvalidateTexAll();

			uint32_t dstOffset = 0;

			for (uint32_t y = 0; y < texHeight; y += 4)
			{
				for (uint32_t x = 0; x < texWidth; x += 4)
				{
					for (uint32_t ty = 0; ty < 4; ty++)
					{
						for (uint32_t tx = 0; tx < 4; tx++)
						{
							Rgba color = render::vscreen->BmpBufPtr1[(y + ty) * texWidth + (x + tx)].rgba;

							textureData[dstOffset] = color.Alpha;
							dstOffset++;

							textureData[dstOffset] = color.Red;
							dstOffset += 31;

							textureData[dstOffset] = color.Green;
							dstOffset++;

							textureData[dstOffset] = color.Blue;
							dstOffset -= 31;

							if ((dstOffset & 31) == 0)
								dstOffset += 32;
						}
					}
				}
			}

			// It's necessary to flush the data cache of the texture after modifying its pixels

			wii_graphics::FlushDataCache(textureData, textureSize);
		}

		// Render fullscreen quad

		wii_graphics::Load2DModelViewMatrix(GX_PNMTX0, render::get_offset_x(), render::get_offset_y());
		wii_graphics::CallDisplayList(boardDisplayList, boardDisplayListSize);

		wii_graphics::Load2DModelViewMatrix(GX_PNMTX0, 0.0f, 0.0f);
		wii_graphics::CallDisplayList(sidebarDisplayList, sidebarDisplayListSize);

		wii_graphics::SwapBuffers();
	}

	printf("Uninitializing...\n");

	end_pause();

	options::uninit();
	midi::music_shutdown();
	pb::uninit();
	Sound::Close();

	free(boardDisplayList);
	free(sidebarDisplayList);
	free(textureData);

	printf("Finished uninitializing.\n");

	return 0;
}

void winmain::memalloc_failure()
{
	midi::music_stop();
	Sound::Close();
	char *caption = pinball::get_rc_string(170, 0);
	char *text = pinball::get_rc_string(179, 0);

	PrintFatalError("%s %s\n", caption, text);
}

void winmain::end_pause()
{
	if (single_step)
	{
		pb::pause_continue();
		no_time_loss = 1;
	}
}

void winmain::new_game()
{
	end_pause();
	pb::replay_level(0);
}

void winmain::pause()
{
	pb::pause_continue();
	no_time_loss = 1;
}

void winmain::UpdateFrameRate()
{
	// UPS >= FPS
	auto fps = Options.FramesPerSecond, ups = Options.UpdatesPerSecond;
	UpdateToFrameRatio = static_cast<double>(ups) / fps;
	TargetFrameTime = DurationMs(1000.0 / ups);
}

void winmain::PrintFatalError(const char *message, ...)
{
	wii_graphics::InitializeConsole();
	wii_graphics::SetNextFramebuffer(0);

	printf("\x1b[4;4H");

	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);

	printf("\x1b[16;30HPress A to exit.");

	while (true)
	{
		wii_input::ScanPads();

		uint32_t wiiButtonsDown = wii_input::GetWiiButtonsDown(0);
		uint32_t gcButtonsDown = wii_input::GetGCButtonsDown(0);

		if ((wiiButtonsDown & WPAD_BUTTON_A) || (gcButtonsDown & PAD_BUTTON_A))
			break;

		VIDEO_Flush();
		VIDEO_WaitVSync();
	}

	VIDEO_SetBlack(true);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	exit(EXIT_FAILURE);
	//svcBreak(USERBREAK_ASSERT);
}
