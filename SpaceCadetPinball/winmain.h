#pragma once
#include "gdrv.h"

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
	static int has_focus, mouse_down, last_mouse_x, last_mouse_y, no_time_loss;
	static gdrv_bitmap8* gfr_display;
	static std::string FpsDetails;
	static bool ShowSpriteViewer;
	static double UpdateToFrameRatio;
	static DurationMs TargetFrameTime;
	static struct optionsStruct& Options;

	static void RenderUi();
};
