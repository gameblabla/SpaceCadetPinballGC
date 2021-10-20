#include "pch.h"
#include "winmain.h"

#include <malloc.h>
#include <wiiuse/wpad.h>

#include "control.h"
#include "midi.h"
#include "options.h"
#include "pb.h"
#include "pinball.h"
#include "render.h"
#include "Sound.h"
#include "wii_graphics.h"

int winmain::return_value = 0;
int winmain::bQuit = 0;
int winmain::activated;
int winmain::DispFrameRate = 0;
int winmain::DispGRhistory = 0;
int winmain::single_step = 0;
int winmain::last_mouse_x;
int winmain::last_mouse_y;
int winmain::mouse_down;
int winmain::no_time_loss;

gdrv_bitmap8 *winmain::gfr_display = nullptr;
std::string winmain::DatFileName;
SDL_Surface *winmain::ScreenSurface = nullptr;
SDL_Joystick *winmain::Joystick = nullptr;
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
	// Initialize graphics

	wii_graphics::Initialize();
	wii_graphics::LoadOrthoProjectionMatrix(0.0f, 1.0f, 0.0f, 1.0f, 0.1f, 10.0f);
	wii_graphics::Load2DModelViewMatrix(GX_PNMTX0, 0.5f, 0.5f);

	// void *displayList = memalign(32, MAX_DISPLAY_LIST_SIZE);
	// uint32_t displayListSize = wii_graphics::Create2DQuadDisplayList(displayList, 0.0f, 1.0f, 0.0f, 1.0f);

	// if (displayListSize == 0)
	// {
	// 	printf("Display list exceeded size.");
	// 	exit(EXIT_FAILURE);
	// }
	// else
	// {
	// 	printf("Display list size: %u", displayListSize);
	// }

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	//GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	//GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGB8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	// Texture data and create texture object

	uint16_t texWidth = 64;
	uint16_t texHeight = 64;
	uint32_t textureSize = wii_graphics::GetTextureSize(texWidth, texHeight, GX_TF_RGBA8, 0, 0);
	GXTexObj textureObject;
	uint8_t *textureData = (uint8_t *)memalign(32, textureSize);
	memset(textureData, 0, textureSize);
	wii_graphics::CreateTextureObject(&textureObject, textureData, texWidth, texHeight, GX_TF_RGBA8, GX_CLAMP, GX_NEAR);
	wii_graphics::LoadTextureObject(&textureObject, GX_TEXMAP0);

	uint32_t offset = 0;
	uint32_t frame = 0;

	WPAD_Init();

	while (1)
	{
		WPAD_ScanPads();

		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
			break;

		GX_InvalidateTexAll();

		frame++;

		wii_graphics::Load2DModelViewMatrix(GX_PNMTX0, (frame & 0xff) / 256.0f, 0.5f);

		// do this before drawing
		//GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);

		offset = 0;
		for (;;)
		{
			textureData[offset] = 0xff; // A
			offset++;

			textureData[offset] = (offset + frame) & 0xff; // R
			offset += 31;

			textureData[offset] = (0x00 + frame) & 0xff; // G
			offset++;

			textureData[offset] = (0xff + frame) & 0xff; // B
			offset -= 31;

			if ((offset & 31) == 0)
				offset += 32;

			if (offset >= textureSize)
				break;
		}

		wii_graphics::FlushDataCache(textureData, textureSize);

		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);

		GX_Position3f32(-0.25f, -0.25f, 0.0f);
		GX_TexCoord2f32(0.0f, 1.0f);

		GX_Position3f32(0.25f, -0.25f, 0.0f);
		GX_TexCoord2f32(1.0f, 1.0f);

		GX_Position3f32(-0.25f, 0.25f, 0.0f);
		GX_TexCoord2f32(0.0f, 0.0f);

		GX_Position3f32(0.25f, 0.25f, 0.0f);
		GX_TexCoord2f32(1.0f, 0.0f);

		GX_End();

		//wii_graphics::CallDisplayList(displayList, displayListSize);
		wii_graphics::SwapBuffers();
	}

	return 0;

	bQuit = false;

	std::set_new_handler(memalloc_failure);

	// SDL init
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		SDL_Delay(5000);
		exit(EXIT_FAILURE);
	}

	atexit(SDL_Quit);
	SDL_ShowCursor(SDL_DISABLE);

	BasePath = (char *)"sd:/apps/SpaceCadetPinball/Data/";

	pinball::quickFlag = 0; // strstr(lpCmdLine, "-quick") != nullptr;
	DatFileName = options::get_string("Pinball Data", pinball::get_rc_string(168, 0));

	/*Check for full tilt .dat file and switch to it automatically*/
	auto cadetFilePath = pinball::make_path_name("CADET.DAT");
	auto cadetDat = fopen(cadetFilePath.c_str(), "r");
	if (cadetDat)
	{
		fclose(cadetDat);
		DatFileName = "CADET.DAT";
		pb::FullTiltMode = true;
	}

	ScreenSurface = SDL_SetVideoMode(640, 480, 16, SDL_DOUBLEBUF);
	if (ScreenSurface == NULL)
	{
		fprintf(stderr, "Unable to set video: %s\n", SDL_GetError());
		SDL_Delay(5000);
		exit(EXIT_FAILURE);
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
			fprintf(stderr, "Could not load game data: The .dat file is missing");
			SDL_Delay(5000);
			exit(EXIT_FAILURE);
		}
	}

	pb::reset_table();
	pb::firsttime_setup();

	//unsigned dtHistoryCounter = 300u, updateCounter = 0, frameCounter = 0;

	pb::replay_level(0);

	//auto frameStart = Clock::now();
	//double UpdateToFrameCounter = 0;
	//DurationMs sleepRemainder(0), frameDuration(TargetFrameTime);
	//auto prevTime = frameStart;

	Joystick = SDL_JoystickOpen(0);
	int hatCount = SDL_JoystickNumHats(Joystick);
	int previousHatState[hatCount];
	for (int h = 0; h < hatCount; h++)
		previousHatState[h] = SDL_HAT_CENTERED;

	while (true)
	{
		// if (DispFrameRate)
		// {
		// 	auto curTime = Clock::now();
		// 	if (curTime - prevTime > DurationMs(1000))
		// 	{
		// 		char buf[60];
		// 		auto elapsedSec = DurationMs(curTime - prevTime).count() * 0.001;
		// 		snprintf(buf, sizeof buf, "Updates/sec = %02.02f Frames/sec = %02.02f ",
		// 		         updateCounter / elapsedSec, frameCounter / elapsedSec);

		// 		FpsDetails = buf;
		// 		frameCounter = updateCounter = 0;
		// 		prevTime = curTime;
		// 	}
		// }

		// if (DispGRhistory)
		// {
		// 	if (!gfr_display)
		// 	{
		// 		auto plt = static_cast<ColorRgba*>(malloc(1024u));
		// 		auto pltPtr = &plt[10];
		// 		for (int i1 = 0, i2 = 0; i1 < 256 - 10; ++i1, i2 += 8)
		// 		{
		// 			unsigned char blue = i2, redGreen = i2;
		// 			if (i2 > 255)
		// 			{
		// 				blue = 255;
		// 				redGreen = i1;
		// 			}

		// 			*pltPtr++ = ColorRgba{Rgba{redGreen, redGreen, blue, 0}};
		// 		}
		// 		gdrv::display_palette(plt);
		// 		free(plt);
		// 		gfr_display = new gdrv_bitmap8(400, 15, false);
		// 	}

		// 	if (!dtHistoryCounter)
		// 	{
		// 		dtHistoryCounter = 300;
		// 		gdrv::copy_bitmap(render::vscreen, 300, 10, 0, 30, gfr_display, 0, 0);
		// 		gdrv::fill_bitmap(gfr_display, 300, 10, 0, 0, 0);
		// 	}
		// }

		if (bQuit)
			break;

		SDL_JoystickUpdate();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			event_handler(&event);
		}

		// SDL_Wii way of handling the DPAD

		for (int h = 0; h < hatCount; h++)
		{
			int hatState = SDL_JoystickGetHat(Joystick, h);
			if (previousHatState[h] == hatState)
				continue;
			previousHatState[h] = hatState;

			switch (hatState)
			{
			case SDL_HAT_UP:
				pb::keydown(PAD_DPAD_UP);
				break;
			case SDL_HAT_RIGHT:
				pb::keydown(PAD_DPAD_RIGHT);
				break;
			case SDL_HAT_LEFT:
				pb::keydown(PAD_DPAD_LEFT);
				break;
			default:
				pb::keyup(PAD_DPAD_LEFT);
				pb::keyup(PAD_DPAD_RIGHT);
				pb::keyup(PAD_DPAD_UP);
				break;
			}
		}

		// if (mouse_down)
		// {
		// 	int x, y;
		// 	SDL_GetMouseState(&x, &y);
		// 	float dx = (last_mouse_x - x) / static_cast<float>(ScreenSurface->w);
		// 	float dy = (y - last_mouse_y) / static_cast<float>(ScreenSurface->h);
		// 	pb::ballset(dx, dy);
		// }

		if (!single_step)
		{
			// auto dt = static_cast<float>(frameDuration.count());
			// auto dtWhole = static_cast<int>(std::round(dt));
			pb::frame(1000.0f / 60.0f);
			// if (gfr_display)
			// {
			// 	auto deltaTPal = dtWhole + 10;
			// 	auto fillChar = static_cast<uint8_t>(deltaTPal);
			// 	if (deltaTPal > 236)
			// 	{
			// 		fillChar = 1;
			// 	}
			// 	gdrv::fill_bitmap(gfr_display, 1, 10, 300 - dtHistoryCounter, 0, fillChar);
			// 	--dtHistoryCounter;
			// }
			// updateCounter++;
		}

		//printf("Current Frame: %u\n", frameCounter);
		// if (UpdateToFrameCounter >= UpdateToFrameRatio)
		// {

		render::PresentVScreen();

		//frameCounter++;
		// UpdateToFrameCounter -= UpdateToFrameRatio;
		// }

		// auto sdlError = SDL_GetError();
		// if (sdlError[0])
		// {
		// 	SDL_ClearError();
		// 	printf("SDL Error: %s\n", sdlError);
		// }

		// auto updateEnd = Clock::now();
		// auto targetTimeDelta = TargetFrameTime - DurationMs(updateEnd - frameStart) - sleepRemainder;

		// TimePoint frameEnd;
		// if (targetTimeDelta > DurationMs::zero() && !Options.UncappedUpdatesPerSecond)
		// {
		// 	std::this_thread::sleep_for(targetTimeDelta);
		// 	frameEnd = Clock::now();
		// 	sleepRemainder = DurationMs(frameEnd - updateEnd) - targetTimeDelta;
		// }
		// else
		// {
		// 	frameEnd = updateEnd;
		// 	sleepRemainder = DurationMs(0);
		// }

		// // Limit duration to 2 * target time
		// frameDuration = std::min<DurationMs>(DurationMs(frameEnd - frameStart), 2 * TargetFrameTime);
		// frameStart = frameEnd;
		// UpdateToFrameCounter++;

		if (SDL_Flip(ScreenSurface) < 0)
		{
			fprintf(stderr, "Error in SDL_Flip: %s\n", SDL_GetError());
			exit(EXIT_FAILURE);
		}
	}

	SDL_JoystickClose(0);

	delete gfr_display;
	options::uninit();
	midi::music_shutdown();
	pb::uninit();
	Sound::Close();
	SDL_Quit();

	return return_value;
}

int winmain::event_handler(const SDL_Event *event)
{
	switch (event->type)
	{
	case SDL_QUIT:
		end_pause();
		bQuit = 1;
		return_value = 0;
		return 0;
	case SDL_JOYBUTTONUP:
		pb::keyup(event->jbutton.button);
		break;
	case SDL_JOYBUTTONDOWN:
		pb::keydown(event->jbutton.button);
		switch (event->jbutton.button)
		{
		case PAD_BUTTON_MINUS:
			new_game();
			break;
		case PAD_BUTTON_PLUS:
			pause();
			break;
		// case SDLK_F4:
		// 	options::toggle(Menu1::Full_Screen);
		// 	break;
		// case SDLK_F5:
		// 	options::toggle(Menu1::Sounds);
		// 	break;
		// case SDLK_F6:
		// 	options::toggle(Menu1::Music);
		// 	break;
		// case SDLK_F9:
		// 	options::toggle(Menu1::Show_Menu);
		// 	break;
		default:
			break;
		}

		// if (!pb::cheat_mode)
		// 	break;

		// switch (event->key.keysym.sym)
		// {
		// case SDLK_g:
		// 	DispGRhistory = 1;
		// 	break;
		// case SDLK_y:
		// 	DispFrameRate = DispFrameRate == 0;
		// 	break;
		// case SDLK_F1:
		// 	pb::frame(10);
		// 	break;
		// case SDLK_F10:
		// 	single_step = single_step == 0;
		// 	if (single_step == 0)
		// 		no_time_loss = 1;
		// 	break;
		// default:
		// 	break;
		// }
		break;
	case SDL_MOUSEBUTTONDOWN:
		switch (event->button.button)
		{
		case SDL_BUTTON_LEFT:
			if (pb::cheat_mode)
			{
				mouse_down = 1;
				last_mouse_x = event->button.x;
				last_mouse_y = event->button.y;
			}
			else
				pb::keydown(Options.Key.LeftFlipper);
			break;
		case SDL_BUTTON_RIGHT:
			if (!pb::cheat_mode)
				pb::keydown(Options.Key.RightFlipper);
			break;
		case SDL_BUTTON_MIDDLE:
			pb::keydown(Options.Key.Plunger);
			break;
		default:
			break;
		}
		break;
	case SDL_MOUSEBUTTONUP:
		switch (event->button.button)
		{
		case SDL_BUTTON_LEFT:
			if (mouse_down)
			{
				mouse_down = 0;
			}
			if (!pb::cheat_mode)
				pb::keyup(Options.Key.LeftFlipper);
			break;
		case SDL_BUTTON_RIGHT:
			if (!pb::cheat_mode)
				pb::keyup(Options.Key.RightFlipper);
			break;
		case SDL_BUTTON_MIDDLE:
			pb::keyup(Options.Key.Plunger);
			break;
		default:
			break;
		}
		break;
	default:;
	}

	return 1;
}

void winmain::memalloc_failure()
{
	midi::music_stop();
	Sound::Close();
	char *caption = pinball::get_rc_string(170, 0);
	char *text = pinball::get_rc_string(179, 0);
	fprintf(stderr, "%s %s\n", caption, text);
	std::exit(1);
	exit(EXIT_FAILURE);
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
