#include "pch.h"
#include "fullscrn.h"


#include "options.h"
#include "pb.h"
#include "render.h"
#include "winmain.h"


int fullscrn::screen_mode;
int fullscrn::display_changed;

int fullscrn::resolution = 0;
const resolution_info fullscrn::resolution_array[3] =
{
	{640, 480, 600, 416, 501},
	{800, 600, 752, 520, 502},
	{1024, 768, 960, 666, 503},
};
float fullscrn::ScaleX = 1;
float fullscrn::ScaleY = 1;
Sint16 fullscrn::OffsetX = 0;
Sint16 fullscrn::OffsetY = 0;

void fullscrn::init()
{
	window_size_changed();
}

int fullscrn::GetResolution()
{
	return resolution;
}

void fullscrn::SetResolution(int value)
{
	if (!pb::FullTiltMode)
		value = 0;
	assertm(value >= 0 && value <= 2, "Resolution value out of bounds");
	resolution = value;
}

int fullscrn::GetMaxResolution()
{
	return pb::FullTiltMode ? 2 : 0;
}

void fullscrn::window_size_changed()
{
	int width = winmain::ScreenSurface->w;
	int height = winmain::ScreenSurface->h;

	auto res = &resolution_array[resolution];
	ScaleX = static_cast<float>(width) / res->TableWidth;
	ScaleY = static_cast<float>(height) / res->TableHeight;
	OffsetX = OffsetY = 0;

	if (options::Options.UniformScaling)
	{
		ScaleY = ScaleX = std::min(ScaleX, ScaleY);
		OffsetX = static_cast<int>(floor((width - res->TableWidth * ScaleX) / 2));
		OffsetY = static_cast<int>(floor((height - res->TableHeight * ScaleY) / 2));
	}

	render::DestinationRect = SDL_Rect
	{
		OffsetX, OffsetY,
		(Uint16)(width - OffsetX * 2), (Uint16)(height - OffsetY * 2)
	};
}
