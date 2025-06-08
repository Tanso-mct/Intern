#include "pch.h"

#include "format_dds.h"
#include "pixel_flipper.h"

using namespace std;

unique_ptr<FileData> DDS::analysis(unique_ptr<u8[]> &importData)
{
    u32 magic = *reinterpret_cast<u32*>(importData.get());
    if (magic != 0x20534444)
    {
        cout << "DDSファイルのマジックナンバーが不正です。" << endl;
        return nullptr;
    }

    DdsHeader* header = reinterpret_cast<DdsHeader*>(importData.get() + sizeof(u32));

    unique_ptr<FileData> fileData = make_unique<FileData>();

    fileData->width = header->width;
    fileData->height = header->height;

    u32 imageSize = fileData->width * fileData->height * 4;
    fileData->pixels = make_unique<u8[]>(imageSize);

    u32 dataOffset = sizeof(u32) + sizeof(DdsHeader);
    if (header->ddspf.fourCC = 0x30315844) dataOffset += sizeof(DdsHeaderDx10); // DDS_HEADER_DX10が存在する
    else
    {
        cout << "DX10ヘッダーが存在しません。DDSファイルはDX10でのみ対応しています。" << endl;
        return nullptr;
    }
    
    DdsHeaderDx10* headerDx10 = reinterpret_cast<DdsHeaderDx10*>(importData.get() + sizeof(u32) + sizeof(DdsHeader));
    if (headerDx10->dxgiFormat != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
    {
        cout << "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB以外のフォーマットは対応していません。" << endl;
        return nullptr;
    }

    PixelFlipper flipper;
    flipper.getFlipTypeToBLTR(PixelStorageOrder::topLeftToBottomRight); // ddsは左上から右下に並んでいる

    flipper.getPixelsFlippedRGBA(importData, dataOffset, imageSize, 32, fileData->pixels, fileData->width, fileData->height);

    return fileData;
}

unique_ptr<u8[]> DDS::convert(unique_ptr<FileData> &fileData, u32 &rtDataSize)
{
    u32 magic = 0x20534444;

    DdsHeader header;
    header.size = sizeof(DdsHeader);
    header.flags = 0x00021007; // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
    header.height = fileData->height;
    header.width = fileData->width;
    header.pitchOrLinearSize = 0;
    header.depth = 0;
    header.mipMapCount = 0;

    header.ddspf.size = sizeof(DdsPixelFormat);
    header.ddspf.flags = 0x00000004;  // DDPF_FOURCC
    header.ddspf.fourCC = 0x30315844; // DX10
    header.ddspf.RGBBitCount = 0;
    header.ddspf.RBitMask = 0;
    header.ddspf.GBitMask = 0;
    header.ddspf.BBitMask = 0;
    header.ddspf.ABitMask = 0;

    header.caps = 0x00000100; // DDSCAPS_TEXTURE
    header.caps2 = 0;
    header.caps3 = 0;
    header.caps4 = 0;
    header.reserved2 = 0;

    DdsHeaderDx10 headerDx10;
    headerDx10.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    headerDx10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
    headerDx10.miscFlag = 0;
    headerDx10.arraySize = 1;
    headerDx10.reserved = 0;

    u32 imageSize = fileData->width * fileData->height * 4;
    rtDataSize = sizeof(u32) + sizeof(DdsHeader) + sizeof(DdsHeaderDx10) + imageSize;

    unique_ptr<u8[]> rtBuff = make_unique<u8[]>(rtDataSize);

    // マジックナンバーを書き込む
    for (size_t i = 0; i < sizeof(u32); ++i) rtBuff[i] = reinterpret_cast<u8*>(&magic)[i];

    // ヘッダー情報を書き込む
    for (size_t i = 0; i < sizeof(DdsHeader); ++i) rtBuff[sizeof(u32) + i] = reinterpret_cast<u8*>(&header)[i];

    // DX10ヘッダー情報を書き込む
    for (size_t i = 0; i < sizeof(DdsHeaderDx10); ++i)
    {
        rtBuff[sizeof(u32) + sizeof(DdsHeader) + i] = reinterpret_cast<u8*>(&headerDx10)[i];
    }

    PixelFlipper flipper;
    flipper.getFlipTypeToTLBR(PixelStorageOrder::bottomLeftToTopRight); // FileDataは左下から右上に並んでいる

    u32 dataOffset = sizeof(u32) + sizeof(DdsHeader) + sizeof(DdsHeaderDx10);

    // ピクセル格納順が合うよう反転させ、BGRAをRGBAに変換して書き込む
    flipper.insertPixelsFlippedRGBA(rtBuff, dataOffset, fileData->pixels, fileData->width, fileData->height);

    return rtBuff;
}
    