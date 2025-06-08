#include "pch.h"

#include "format_tga.h"

#include "pixel_flipper.h"

using namespace std;

unique_ptr<FileData> TGA::analysis(unique_ptr<u8[]> &importData)
{
    TgaFileHeader* fileHeader = reinterpret_cast<TgaFileHeader*>(importData.get());

    unique_ptr<FileData> fileData = make_unique<FileData>();

    fileData->width = fileHeader->width;
    fileData->height = fileHeader->height;

    u32 size = fileData->width * fileData->height * 4;
    fileData->pixels = make_unique<u8[]>(size);

    u32 dataOffset = sizeof(TgaFileHeader) + fileHeader->idLength;

    // ビット5とビット4を取得
    u8 bit5 = (fileHeader->imageDescriptor >> 5) & 1;
    u8 bit4 = (fileHeader->imageDescriptor >> 4) & 1;

    PixelStorageOrder order;
    if (bit5 == 0 && bit4 == 0) order = PixelStorageOrder::bottomLeftToTopRight;
    else if (bit5 == 0 && bit4 == 1) order = PixelStorageOrder::topLeftToBottomRight;
    else if (bit5 == 1 && bit4 == 0) order = PixelStorageOrder::bottomRightToTopLeft;
    else if (bit5 == 1 && bit4 == 1) order = PixelStorageOrder::topRightToBottomLeft;
    else
    {
        cout << "画像記述子のビット5とビット4が不正です。" << endl;
        return nullptr;
    }

    PixelFlipper flipper;
    flipper.getFlipTypeToBLTR(order);

    if (fileHeader->imageType == 2)
    {
        flipper.getPixelsFlippedBGRA
        (
            importData, dataOffset, size, fileHeader->pixelDepth,
            fileData->pixels, fileData->width, fileData->height
        );
    }
    else if (fileHeader->imageType == 10)
    {
        unique_ptr<u8[]> uncompressedData = uncompress
        (
            importData, dataOffset, 
            fileData->width, fileData->height, fileHeader->pixelDepth
        );

        flipper.getPixelsFlippedBGRA
        (
            uncompressedData, 0, size, 32,
            fileData->pixels, fileData->width, fileData->height
        );
    }

    return fileData;
}

unique_ptr<u8[]> TGA::convert(unique_ptr<FileData> &fileData, u32 &rtDataSize)
{
    TgaFileHeader fileHeader;
    fileHeader.idLength = 0;
    fileHeader.colorMapType = 0;
    fileHeader.imageType = (useCompression_) ? 10 : 2;
    fileHeader.colorMapIndex = 0;
    fileHeader.colorMapLength = 0;
    fileHeader.colorMapDepth = 0;
    fileHeader.xOrigin = 0;
    fileHeader.yOrigin = 0;
    fileHeader.width = fileData->width;
    fileHeader.height = fileData->height;
    fileHeader.pixelDepth = 32;
    fileHeader.imageDescriptor = 0; // bottom left to top right

    u32 imageSize = fileData->width * fileData->height * 4;

    vector<u8> compressedData;
    if (useCompression_) // 圧縮する場合
    {
        compressedData = compress(fileData); // 圧縮
        rtDataSize = sizeof(TgaFileHeader) + compressedData.size();
    }
    else rtDataSize = sizeof(TgaFileHeader) + imageSize; // 圧縮しない場合

	unique_ptr<u8[]> rtBuff = make_unique<u8[]>(rtDataSize);

    // ヘッダー情報を書き込む
    for (size_t i = 0; i < sizeof(TgaFileHeader); ++i) 
	{
        rtBuff[i] = reinterpret_cast<u8*>(&fileHeader)[i];
    }

    // ピクセルデータを書き込む
    if (useCompression_) // 圧縮する場合
    {
        for (u32 i = 0; i < compressedData.size(); i++)
        {
            rtBuff[sizeof(TgaFileHeader) + i] = compressedData[i];
        }
    }
    else
    {
        for (u32 i = 0; i < imageSize; i++)
        {
            rtBuff[sizeof(TgaFileHeader) + i] = fileData->pixels[i];
        }
    }

    return rtBuff;
}

namespace
{

u32 GetIndex(u32 x, u32 y, s32 width, s32 height)
{
    return y * width * 4 + x * 4;
}

}

unique_ptr<u8[]> TGA::uncompress(unique_ptr<u8[]> &importData, u32 dataOffset, s32 width, s32 height, u16 pixelDepth)
{
    u32 maxAlpha = 255;
    unique_ptr<u8[]> pixels = make_unique<u8[]>(width * height * 4);

    u8* src = importData.get() + dataOffset;
    u16 clrWidth = pixelDepth / 8;

    for (u32 y = 0; y < height; y++)
    {
        for (u32 x = 0; x < width;)
        {
            if (*src & 0x80) // Repeat
            {
                u32 count = (*src & 0x7F) + 1;
                src++;

                BGRA* pixel = reinterpret_cast<BGRA*>(src);
                for (u32 i = 0; i < count; i++)
                {
                    assert(y*width*4 + x + 3 < width*height*4);

                    pixels[GetIndex(x, y, width, height) ] = pixel->b;
                    pixels[GetIndex(x, y, width, height) + 1] = pixel->g;
                    pixels[GetIndex(x, y, width, height) + 2] = pixel->r;
                    pixels[GetIndex(x, y, width, height) + 3] = (clrWidth == 4) ? pixel->a : 0xff;
                    x++;
                }

                src += clrWidth;
            }
            else // Literal
            {
                u32 count = (*src & 0x7F) + 1;
                src++;

                for (u32 i = 0; i < count; i++)
                {
                    assert(y*width*4 + x + 3 < width*height*4);

                    BGRA* pixel = reinterpret_cast<BGRA*>(src);
                    pixels[GetIndex(x, y, width, height)] = pixel->b;
                    pixels[GetIndex(x, y, width, height) + 1] = pixel->g;
                    pixels[GetIndex(x, y, width, height) + 2] = pixel->r;
                    pixels[GetIndex(x, y, width, height) + 3] = (clrWidth == 4) ? pixel->a : 0xff;

                    src += clrWidth;
                    x++;
                }
            }

            if (x >= width) break;
        }
    }

    return pixels;
}

namespace
{

bool IsSameBGRA(BGRA* a, BGRA* b)
{
    return a->b == b->b && a->g == b->g && a->r == b->r && a->a == b->a;
}

}

vector<u8> TGA::compress(unique_ptr<FileData> &fileData)
{
    vector<u8> compressData;

    u32 runMaxLen = 128;
    for (u32 y = 0; y < fileData->height; y++)
    {
        for (u32 x = 0; x < fileData->width;)
        {
            u8* thisPixelStart = &fileData->pixels[y*fileData->width*4 + x*4];
            BGRA* thisPixel = reinterpret_cast<BGRA*>(thisPixelStart);

            if (x + 1 == fileData->width) // 判定するべき次のピクセルがない場合はLiteral
            {
                u32 count = 1;
                compressData.push_back(count - 1);
                compressData.push_back(thisPixel->b);
                compressData.push_back(thisPixel->g);
                compressData.push_back(thisPixel->r);
                compressData.push_back(thisPixel->a);
                
                break;
            }

            u8* nextPixelStart = thisPixelStart + 4;
            BGRA* nextPixel = reinterpret_cast<BGRA*>(nextPixelStart);

            if (IsSameBGRA(thisPixel, nextPixel)) // Repeat
            {
                u32 count = 1;
                for (u32 i = 0; i < runMaxLen; i++)
                {
                    if (x + i >= fileData->width) break;

                    BGRA* judgePixel = reinterpret_cast<BGRA*>(nextPixelStart + i*4);
                    if (IsSameBGRA(thisPixel, judgePixel)) count++;
                    else break;
                }

                compressData.push_back(0x80 | (count - 1));
                compressData.push_back(thisPixel->b);
                compressData.push_back(thisPixel->g);
                compressData.push_back(thisPixel->r);
                compressData.push_back(thisPixel->a);

                x += count;
            }
            else // Literal
            {
                u32 count = 0;
                for (u32 i = 0; i < runMaxLen; i++)
                {
                    if (x + i >= fileData->width) break;

                    BGRA* judgePixel = reinterpret_cast<BGRA*>(nextPixelStart + i*4);
                    if (!IsSameBGRA(thisPixel, judgePixel))
                    {
                        count++;
                        thisPixel = judgePixel;
                    }
                    else break;
                }

                compressData.push_back(count - 1);
                for (u32 i = 0; i < count; i++)
                {
                    BGRA* pixel = reinterpret_cast<BGRA*>(thisPixelStart + i*4);
                    compressData.push_back(pixel->b);
                    compressData.push_back(pixel->g);
                    compressData.push_back(pixel->r);
                    compressData.push_back(pixel->a);
                }

                x += count;
            }

            if (x >= fileData->width) break;
        }
    }

    return compressData;
}
