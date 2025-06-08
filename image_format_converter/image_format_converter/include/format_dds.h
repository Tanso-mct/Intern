#pragma once

#include <memory>
#include <string>

#include <d3d11.h>

#include "converter.h"

#pragma pack(push, 1)
struct DdsPixelFormat
{
    u32 size;        // 構造体のサイズ（常に32）
    u32 flags;       // ピクセルフォーマットフラグ
    u32 fourCC;      // 圧縮形式（例: DXT1, DX10など）
    u32 RGBBitCount; // ビット深度（非圧縮形式の場合）
    u32 RBitMask;    // Rチャネルのビットマスク
    u32 GBitMask;    // Gチャネルのビットマスク
    u32 BBitMask;    // Bチャネルのビットマスク
    u32 ABitMask;    // Aチャネルのビットマスク
};

// DDS_HEADER構造体の定義
struct DdsHeader 
{
    u32 size;              // 構造体のサイズ（常に124）
    u32 flags;             // DDSファイルの特性を示すフラグ
    u32 height;            // 画像の高さ
    u32 width;             // 画像の幅
    u32 pitchOrLinearSize; // 行ピッチまたは圧縮データサイズ
    u32 depth;             // 深さ（3Dテクスチャの場合）
    u32 mipMapCount;       // ミップマップの数
    u32 reserved1[11];     // 予約領域
    DdsPixelFormat ddspf;  // ピクセルフォーマット構造体
    u32 caps;              // サーフェスキャップ
    u32 caps2;             // 追加キャップ（キューブマップなど）
    u32 caps3;             // 予約
    u32 caps4;             // 予約
    u32 reserved2;         // 予約
};

// DDS_HEADER_DX10構造体の定義
struct DdsHeaderDx10 
{
    DXGI_FORMAT dxgiFormat; // DXGIフォーマット
    D3D10_RESOURCE_DIMENSION resourceDimension; // リソースの次元（例: 2D, 3D）
    u32 miscFlag; // キューブマップなどの特性
    u32 arraySize; // 配列テクスチャの要素数
    u32 reserved; // 予約領域
};
#pragma pack(pop)

class DDS : public IConverter
{
public:
    DDS() : IConverter("dds") {}
    ~DDS() final = default;

    std::unique_ptr<FileData> analysis(std::unique_ptr<u8[]>& importData) final;
    std::unique_ptr<u8[]> convert(std::unique_ptr<FileData>& fileData, u32& rtDataSize) final;
};