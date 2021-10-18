#include "pch.h"
#include "zdrv.h"
#include "winmain.h"


zmap_header_type::zmap_header_type(int width, int height, int stride)
{
	Resolution = 0;
	Width = width;
	Height = height;
	Stride = stride >= 0 ? stride : pad(width);
	Texture = nullptr;
	ZPtr1 = new unsigned short[Stride * Height];
	RgbaBufPtr1 = new ColorRgba[Width * Height];
}

zmap_header_type::~zmap_header_type()
{
	if (Texture)
		SDL_FreeSurface(Texture);
	delete[] ZPtr1;
	delete[] RgbaBufPtr1;
}

int zmap_header_type::pad(int width)
{
	int result = width;
	if (width & 3)
		result = width - (width & 3) + 4;
	return result;
}


void zdrv::fill(zmap_header_type* zmap, int width, int height, int xOff, int yOff, uint16_t fillWord)
{
	auto dstPtr = &zmap->ZPtr1[zmap->Stride * yOff + xOff];
	for (int y = height; y > 0; --y)
	{
		for (int x = width; x > 0; --x)
		{
			*dstPtr++ = fillWord;
		}
		dstPtr += zmap->Stride - width;
	}
}


void zdrv::paint(int width, int height, gdrv_bitmap8* dstBmp, int dstBmpXOff, int dstBmpYOff, zmap_header_type* dstZMap,
                 int dstZMapXOff, int dstZMapYOff, gdrv_bitmap8* srcBmp, int srcBmpXOff, int srcBmpYOff,
                 zmap_header_type* srcZMap, int srcZMapXOff, int srcZMapYOff)
{
	assertm(srcBmp->BitmapType != BitmapTypes::Spliced, "Wrong bmp type");

	auto srcPtr = &srcBmp->BmpBufPtr1[srcBmp->Stride * srcBmpYOff + srcBmpXOff];
	auto dstPtr = &dstBmp->BmpBufPtr1[dstBmp->Stride * dstBmpYOff + dstBmpXOff];
	auto srcPtrZ = &srcZMap->ZPtr1[srcZMap->Stride * srcZMapYOff + srcZMapXOff];
	auto dstPtrZ = &dstZMap->ZPtr1[dstZMap->Stride * dstZMapYOff + dstZMapXOff];

	for (int y = height; y > 0; y--)
	{
		for (int x = width; x > 0; --x)
		{
			if (*dstPtrZ >= *srcPtrZ)
			{
				*dstPtr = *srcPtr;
				*dstPtrZ = *srcPtrZ;
			}
			++srcPtr;
			++dstPtr;
			++srcPtrZ;
			++dstPtrZ;
		}

		srcPtr += srcBmp->Stride - width;
		dstPtr += dstBmp->Stride - width;
		srcPtrZ += srcZMap->Stride - width;
		dstPtrZ += dstZMap->Stride - width;
	}
}

void zdrv::paint_flat(int width, int height, gdrv_bitmap8* dstBmp, int dstBmpXOff, int dstBmpYOff,
                      zmap_header_type* zMap, int dstZMapXOff, int dstZMapYOff, gdrv_bitmap8* srcBmp, int srcBmpXOff,
                      int srcBmpYOff, uint16_t depth)
{
	assertm(srcBmp->BitmapType != BitmapTypes::Spliced, "Wrong bmp type");

	auto dstPtr = &dstBmp->BmpBufPtr1[dstBmp->Stride * dstBmpYOff + dstBmpXOff];
	auto srcPtr = &srcBmp->BmpBufPtr1[srcBmp->Stride * srcBmpYOff + srcBmpXOff];
	auto zPtr = &zMap->ZPtr1[zMap->Stride * dstZMapYOff + dstZMapXOff];

	for (int y = height; y > 0; y--)
	{
		for (int x = width; x > 0; --x)
		{
			if ((*srcPtr).Color && *zPtr > depth)
			{
				*dstPtr = *srcPtr;
			}
			++srcPtr;
			++dstPtr;
			++zPtr;
		}

		srcPtr += srcBmp->Stride - width;
		dstPtr += dstBmp->Stride - width;
		zPtr += zMap->Stride - width;
	}
}

void zdrv::CreatePreview(zmap_header_type& zMap)
{
	if (zMap.Texture)
		return;

	ColorRgba color{};
	auto dst = zMap.RgbaBufPtr1;
	auto src = zMap.ZPtr1;
	for (auto y = 0; y < zMap.Height; y++)
	{
		for (auto x = 0; x < zMap.Width; x++)
		{
			auto depth = static_cast<uint8_t>((0xffff - *src++) / 0xff);
			color.rgba.Blue = depth;
			color.rgba.Green = depth;
			color.rgba.Red = depth;
			*dst++ = color;
		}
		src += zMap.Stride - zMap.Width;
	}

	auto texture = SDL_CreateRGBSurfaceFrom
	(
		zMap.RgbaBufPtr1,
		zMap.Width,
		zMap.Height,
		32,
		zMap.Width * 4,
#if BIG_ENDIAN
		0xff000000,
		0x00ff0000,
		0x0000ff00,
		0x000000ff
#else
		0x000000ff,
		0x0000ff00,
		0x00ff0000,
		0xff000000
#endif
	);

	zMap.Texture = texture;
}

void zdrv::FlipZMapHorizontally(const zmap_header_type& zMap)
{
	// Flip in-place, iterate over Height/2 lines
	auto dst = zMap.ZPtr1;
	auto src = zMap.ZPtr1 + zMap.Stride * (zMap.Height - 1);
	for (auto y = zMap.Height - 1; y >= zMap.Height / 2; y--)
	{
		for (auto x = 0; x < zMap.Width; x++)
		{
			std::swap(*dst++, *src++);
		}
		dst += zMap.Stride - zMap.Width;
		src -= zMap.Stride + zMap.Width;
	}
}
