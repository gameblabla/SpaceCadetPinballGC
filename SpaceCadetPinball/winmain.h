#pragma once
#include "gdrv.h"

// SDL Buttons
#define PAD_BUTTON_A 0
#define PAD_BUTTON_B 1
#define PAD_BUTTON_1 2
#define PAD_BUTTON_2 3
#define PAD_BUTTON_MINUS 4
#define PAD_BUTTON_PLUS 5
#define PAD_BUTTON_Z 7
#define PAD_BUTTON_C 8

// Custom DPAD defines, // as SDL values conflict
// between buttons and DPAD (handled as "hats")
#define PAD_DPAD_LEFT 256
#define PAD_DPAD_RIGHT 257
#define PAD_DPAD_UP 258
#define PAD_DPAD_DOWN 259

struct SdlTickClock
{
	using duration = std::chrono::milliseconds;
	using rep = duration::rep;
	using period = duration::period;
	using time_point = std::chrono::time_point<SdlTickClock>;
	static constexpr bool is_steady = true;

	static time_point now() noexcept
	{
		return time_point{duration{SDL_GetTicks()}};
	}
};

class winmain
{
	using Clock = std::chrono::steady_clock;
	using DurationMs = std::chrono::duration<double, std::milli>;
	using TimePoint = std::chrono::time_point<Clock>;

public:
	static std::string DatFileName;
	static int single_step;
	static SDL_Surface* ScreenSurface;
	static SDL_Joystick* Joystick;
	static bool LaunchBallEnabled;
	static bool HighScoresEnabled;
	static bool DemoActive;
	static char* BasePath;

	static int WinMain(LPCSTR lpCmdLine);
	static int event_handler(const SDL_Event* event);
	[[ noreturn ]] static void memalloc_failure();
	static void end_pause();
	static void new_game();
	static void pause();
	static void UpdateFrameRate();
private:
	static int return_value, bQuit, DispFrameRate, DispGRhistory, activated;
	static int mouse_down, last_mouse_x, last_mouse_y, no_time_loss;
	static gdrv_bitmap8* gfr_display;
	static std::string FpsDetails;
	static bool ShowSpriteViewer;
	static double UpdateToFrameRatio;
	static DurationMs TargetFrameTime;
	static struct optionsStruct& Options;
};
