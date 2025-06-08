#pragma once

#include <memory>
#include <string_view>
#include <string>

#include <map>

#include "type.h"

#pragma pack(push, 1)
struct BGRA
{
    u8 b;
    u8 g;
    u8 r;
    u8 a;
};

struct RGBA
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};
#pragma pack(pop)

class FileData
{
public :
    s32 width = 0;
    s32 height = 0;
    std::unique_ptr<u8[]> pixels = nullptr;
};

class IConverter
{
private :
    const std::string ext_;
public :
    IConverter(std::string ext) : ext_(ext) {}
    virtual ~IConverter() = default;
    
    bool judgeExt(std::string_view importPath);
    virtual std::unique_ptr<u8[]> load(std::string_view importPath);
    virtual std::unique_ptr<FileData> analysis(std::unique_ptr<u8[]>& importData) = 0;
    virtual std::unique_ptr<u8[]> convert(std::unique_ptr<FileData>& fileData, u32& rtDataSize) = 0;
    virtual u32 write(std::string_view exportPath, u8 *data, const u32 dataSize);
};

class Converter
{
private :
    std::map<std::string, std::unique_ptr<IConverter>> observers_;

public :
    Converter() = default;
    ~Converter() = default;

    void addObserver(std::string ext, std::unique_ptr<IConverter> observer);
    
    std::unique_ptr<FileData> fileAnalysis(std::string_view importPath);
    u32 fileConvert(std::string_view exportPath, std::unique_ptr<FileData> &fileData);
};