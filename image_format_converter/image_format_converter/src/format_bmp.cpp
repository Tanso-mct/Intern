#include "pch.h"

#include "format_bmp.h"

#include "pixel_flipper.h"

using namespace std;

unique_ptr<FileData> BMP::analysis(unique_ptr<u8[]> &importData)
{
    BmpFileHeader* fileHeader = reinterpret_cast<BmpFileHeader*>(importData.get());
    BmpInfoHeader* infoHeader = reinterpret_cast<BmpInfoHeader*>(importData.get() + sizeof(BmpFileHeader));

    // BMPファイルであることを確認
    if (fileHeader->fileType != 0x4d42) return nullptr;

    unique_ptr<FileData> fileData = make_unique<FileData>();

    fileData->width = infoHeader->width;
    fileData->height = infoHeader->height;
    u32 size = fileData->width * abs(fileData->height) * 4;
    fileData->pixels = make_unique<u8[]>(size);

    // BMPファイルのピクセルデータの格納順を取得
    PixelStorageOrder order;
    if (fileData->height > 0) order = PixelStorageOrder::bottomLeftToTopRight;
    else order = PixelStorageOrder::topLeftToBottomRight;

    PixelFlipper flipper;
    flipper.getFlipTypeToBLTR(order);

    if (infoHeader->compression == 3 ||infoHeader->compression == 0 )
    {
        flipper.getPixelsFlippedWithPadBGRA
        (
            importData, fileHeader->fileOffBits, size, infoHeader->pixelDepth,
            fileData->pixels, fileData->width, fileData->height
        );
    }

    return fileData;
}

unique_ptr<u8[]> BMP::convert(unique_ptr<FileData> &fileData, u32 &rtDataSize)
{
    BmpFileHeader fileHeader;
    fileHeader.fileType = 0x4d42; // BM
    fileHeader.fileSize = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) + fileData->width * fileData->height * 4;
    fileHeader.fileReserved1 = 0;
    fileHeader.fileReserved2 = 0;
    fileHeader.fileOffBits = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader);

	rtDataSize = fileHeader.fileSize;

    BmpInfoHeader infoHeader;
    infoHeader.size = sizeof(BmpInfoHeader);
    infoHeader.width = fileData->width;
    infoHeader.height = abs(fileData->height); // bottom left to top right
    infoHeader.planes = 1;
    infoHeader.pixelDepth = 32;
    infoHeader.compression = 0;
    infoHeader.sizeImage = fileData->width * fileData->height * 4;
    infoHeader.xDpi = 0;
    infoHeader.yDpi = 0;
    infoHeader.clrUsed = 0;
    infoHeader.clrImportant = 0;

    unique_ptr<u8[]> rtBuff = make_unique<u8[]>(rtDataSize);

    // ヘッダー情報を書き込む
    for (size_t i = 0; i < sizeof(BmpFileHeader); ++i)
	{
        rtBuff[i] = reinterpret_cast<u8*>(&fileHeader)[i];
    }

    // インフォヘッダー情報を書き込む
    for (size_t i = 0; i < sizeof(BmpInfoHeader); ++i) 
	{
        rtBuff[sizeof(BmpFileHeader) + i] = reinterpret_cast<u8*>(&infoHeader)[i];
    }

    // ピクセルデータを書き込む
    for (u32 i = 0; i < infoHeader.sizeImage; i += 4)
	{
		rtBuff[fileHeader.fileOffBits + i] = fileData->pixels[i];
		rtBuff[fileHeader.fileOffBits + i + 1] = fileData->pixels[i + 1];
		rtBuff[fileHeader.fileOffBits + i + 2] = fileData->pixels[i + 2];
		rtBuff[fileHeader.fileOffBits + i + 3] = fileData->pixels[i + 3];
	}

    return rtBuff;
}