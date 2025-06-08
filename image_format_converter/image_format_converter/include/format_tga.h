#pragma once

#include <string>
#include <vector>

#include "converter.h"

#pragma pack(push, 1)
struct TgaFileHeader
{
    u8 idLength;        // IDフィールドの長さ
    u8 colorMapType;    // カラーマップの有無
    u8 imageType;       // 画像タイプ
    u16 colorMapIndex;  // カラーマップの開始インデックス
    u16 colorMapLength; // カラーマップのエントリ数
    u8 colorMapDepth;   // カラーマップのエントリのビット数
    u16 xOrigin;        // 画像のX座標
    u16 yOrigin;        // 画像のY座標
    u16 width;          // 画像の幅
    u16 height;         // 画像の高さ
    u8 pixelDepth;      // ピクセルのビット深度
    u8 imageDescriptor; // 画像記述子
};
#pragma pack(pop)

class TGA : public IConverter
{
private:
    bool useCompression_ = false;

public:
    TGA(bool useCompression = false) : useCompression_(useCompression), IConverter("tga") {}
    ~TGA() final = default;

    std::unique_ptr<FileData> analysis(std::unique_ptr<u8[]>& importData) final;
    std::unique_ptr<u8[]> convert(std::unique_ptr<FileData>& fileData, u32& rtDataSize) final;

    std::unique_ptr<u8[]> uncompress(std::unique_ptr<u8[]>& importData, u32 dataOffset, s32 width, s32 height, u16 pixelDepth);
    std::vector<u8> compress(std::unique_ptr<FileData>& fileData);
};