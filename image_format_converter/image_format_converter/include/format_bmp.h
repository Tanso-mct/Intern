#pragma once

#include <string>

#include "converter.h"

#pragma pack(push, 1)
struct BmpFileHeader
{
    u16 fileType;         // ファイルタイプ
    u32 fileSize;         // ファイルサイズ
    u16 fileReserved1;    // 予約領域
    u16 fileReserved2;    // 予約領域
    u32 fileOffBits;      // ファイル先頭から画像データまでのオフセット
};

struct BmpInfoHeader
{
    u32 size;         // 構造体のサイズ
    s32 width;        // 画像の幅
    s32 height;       // 画像の高さ
    u16 planes;       // プレーン数
    u16 pixelDepth;     // ビット数
    u32 compression;  // 圧縮形式
    u32 sizeImage;    // 画像データのサイズ
    s32 xDpi;         // 水平解像度
    s32 yDpi;         // 垂直解像度
    u32 clrUsed;      // カラーパレット数
    u32 clrImportant; // 重要なカラー数
};
#pragma pack(pop)

class BMP : public IConverter
{
public:
    BMP() : IConverter("bmp") {}
    ~BMP() override = default;

    std::unique_ptr<FileData> analysis(std::unique_ptr<u8[]>& importData) final;
    std::unique_ptr<u8[]> convert(std::unique_ptr<FileData>& fileData, u32& rtDataSize) final;
};