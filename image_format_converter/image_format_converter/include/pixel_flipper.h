#pragma once

#include <memory>

#include "type.h"

enum FlippedType
{
    none = 0,
    x,
    y,
    xy,
};

enum PixelStorageOrder
{
    bottomLeftToTopRight = 0,
    topLeftToBottomRight,
    bottomRightToTopLeft,
    topRightToBottomLeft,
};

class PixelFlipper
{
private :
    FlippedType type_;
    
public :
    PixelFlipper() = default;
    ~PixelFlipper() = default;

    // ピクセルの並びを画像の左下から右上に変換するようなフリップタイプを取得
    void getFlipTypeToBLTR(PixelStorageOrder order);

    // ピクセルの並びを画像の左上から右下に変換するようなフリップタイプを取得
    void getFlipTypeToTLBR(PixelStorageOrder order);

    void getPixelsFlippedWithPadBGRA
    (
        std::unique_ptr<u8[]>& src, u32 dataOffset, u32 imageSize, u16 pixelDepth, 
        std::unique_ptr<u8[]>& pixels, s32 width, s32 height
    );

    void getPixelsFlippedBGRA
    (
        std::unique_ptr<u8[]>& src, u32 dataOffset, u32 imageSize, u16 pixelDepth, 
        std::unique_ptr<u8[]>& pixels, s32 width, s32 height
    );

    void getPixelsFlippedRGBA
    (
        std::unique_ptr<u8[]>& src, u32 dataOffset, u32 imageSize, u16 pixelDepth, 
        std::unique_ptr<u8[]>& pixels, s32 width, s32 height
    );

    void insertPixelsFlippedRGBA
    (
        std::unique_ptr<u8[]> &target, u32 dataOffset, 
        std::unique_ptr<u8[]> &pixels, s32 width, s32 height
    );
};