#include "pch.h"

#include "pixel_flipper.h"

#include "converter.h"

using namespace std;

void PixelFlipper::getFlipTypeToBLTR(PixelStorageOrder order)
{
    // bottom left to top rightに合わせるようにする
	switch (order)
	{
	case PixelStorageOrder::bottomRightToTopLeft:
		type_ = FlippedType::x;
        break;

	case PixelStorageOrder::topLeftToBottomRight:
		type_ = FlippedType::y;
        break;

	case PixelStorageOrder::topRightToBottomLeft:
		type_ = FlippedType::xy;
        break;

    default:
        type_ = FlippedType::none;
        break;
	}
}

void PixelFlipper::getFlipTypeToTLBR(PixelStorageOrder order)
{
    // top left to bottom rightに合わせるようにする
    switch (order)
    {
    case PixelStorageOrder::bottomLeftToTopRight:
        type_ = FlippedType::y;
        break;

    case PixelStorageOrder::topRightToBottomLeft:
        type_ = FlippedType::x;
        break;

    case PixelStorageOrder::bottomRightToTopLeft:
        type_ = FlippedType::xy;
        break;

    default:
        type_ = FlippedType::none;
        break;
    }
}

void PixelFlipper::getPixelsFlippedWithPadBGRA(
    unique_ptr<u8[]> &src, u32 dataOffset, u32 imageSize, u16 pixelDepth,
    unique_ptr<u8[]> &pixels, s32 width, s32 height)
{
    u16 clrWidth = pixelDepth / 8;
    u32 srcIndex = dataOffset;

    for (u32 i = 0; i < imageSize; i += 4)
    {
        u32 x = (i / 4) % width;
        u32 y = (i / 4) / width;

        u32 flippedIndex;
        switch (type_)
        {
        case FlippedType::x:
            flippedIndex = y * width * 4 + (width - x - 1) * 4;
            break;
        case FlippedType::y:
            flippedIndex = (height - y - 1) * width * 4 + x * 4;
            break;
        case FlippedType::xy:
            flippedIndex = (height - y - 1) * width * 4 + (width - x - 1) * 4;
            break;
        default:
            flippedIndex = i;
            break;
        }

        assert(flippedIndex < imageSize);

        BGRA* pixel = reinterpret_cast<BGRA*>(&src[srcIndex]);
        pixels[flippedIndex] = pixel->b;
        pixels[flippedIndex + 1] = pixel->g;
        pixels[flippedIndex + 2] = pixel->r;
        pixels[flippedIndex + 3] = (clrWidth == 4) ? pixel->a : 0xff;

        srcIndex += clrWidth;

		// 行末の場合は、パディングを考慮
		if (x == width - 1) srcIndex += (4 - (width * clrWidth) % 4) % 4;
    }
}

void PixelFlipper::getPixelsFlippedBGRA
(
    unique_ptr<u8[]> &src, u32 dataOffset, u32 imageSize, u16 pixelDepth, 
    unique_ptr<u8[]> &pixels, s32 width, s32 height
){
    u16 clrWidth = pixelDepth / 8;
    u32 srcIndex = dataOffset;

    for (u32 i = 0; i < imageSize; i += 4)
    {
        u32 x = (i / 4) % width;
        u32 y = (i / 4) / width;

        u32 flippedIndex;
        switch (type_)
        {
        case FlippedType::x:
            flippedIndex = y * width * 4 + (width - x - 1) * 4;
            break;
        case FlippedType::y:
            flippedIndex = (height - y - 1) * width * 4 + x * 4;
            break;
        case FlippedType::xy:
            flippedIndex = (height - y - 1) * width * 4 + (width - x - 1) * 4;
            break;
        default:
            flippedIndex = i;
            break;
        }

        assert(flippedIndex < imageSize);

        BGRA* pixel = reinterpret_cast<BGRA*>(&src[srcIndex]);
        pixels[flippedIndex] = pixel->b;
        pixels[flippedIndex + 1] = pixel->g;
        pixels[flippedIndex + 2] = pixel->r;
        pixels[flippedIndex + 3] = (clrWidth == 4) ? pixel->a : 0xff;

        srcIndex += clrWidth;
    }
}

void PixelFlipper::getPixelsFlippedRGBA
(
    unique_ptr<u8[]> &src, u32 dataOffset, u32 imageSize, u16 pixelDepth, 
    unique_ptr<u8[]> &pixels, s32 width, s32 height
){
    u16 clrWidth = pixelDepth / 8;
    u32 srcIndex = dataOffset;

    for (u32 i = 0; i < imageSize; i += 4)
    {
        u32 x = (i / 4) % width;
        u32 y = (i / 4) / width;

        u32 flippedIndex;
        switch (type_)
        {
        case FlippedType::x:
            flippedIndex = y * width * 4 + (width - x - 1) * 4;
            break;
        case FlippedType::y:
            flippedIndex = (height - y - 1) * width * 4 + x * 4;
            break;
        case FlippedType::xy:
            flippedIndex = (height - y - 1) * width * 4 + (width - x - 1) * 4;
            break;
        default:
            flippedIndex = i;
            break;
        }

        assert(flippedIndex < imageSize);

        RGBA* pixel = reinterpret_cast<RGBA*>(&src[srcIndex]);
        pixels[flippedIndex] = pixel->b;
        pixels[flippedIndex + 1] = pixel->g;
        pixels[flippedIndex + 2] = pixel->r;
        pixels[flippedIndex + 3] = (clrWidth == 4) ? pixel->a : 0xff;

        srcIndex += clrWidth;
    }
}

void PixelFlipper::insertPixelsFlippedRGBA
(
    unique_ptr<u8[]> &target, u32 dataOffset, 
    unique_ptr<u8[]> &pixels, s32 width, s32 height
){
    u16 clrWidth = 4;
    u32 pixelIndex = 0;
    u32 imageSize = width * height * 4;

    for (u32 i = 0; i < imageSize; i += 4)
    {
        u32 x = (i / 4) % width;
        u32 y = (i / 4) / width;

        u32 flippedIndex;
        switch (type_)
        {
        case FlippedType::x:
            flippedIndex = y * width * 4 + (width - x - 1) * 4;
            break;
        case FlippedType::y:
            flippedIndex = (height - y - 1) * width * 4 + x * 4;
            break;
        case FlippedType::xy:
            flippedIndex = (height - y - 1) * width * 4 + (width - x - 1) * 4;
            break;
        default:
            flippedIndex = i;
            break;
        }

        assert(flippedIndex < imageSize);

        BGRA* pixel = reinterpret_cast<BGRA*>(&pixels[pixelIndex]);
        target[dataOffset + flippedIndex] = pixel->r;
        target[dataOffset + flippedIndex + 1] = pixel->g;
        target[dataOffset + flippedIndex + 2] = pixel->b;
        target[dataOffset + flippedIndex + 3] = pixel->a;

        pixelIndex += clrWidth;
    }
}
