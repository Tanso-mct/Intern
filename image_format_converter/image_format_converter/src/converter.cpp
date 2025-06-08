#include "pch.h"

#include "converter.h"

using namespace std;

bool IConverter::judgeExt(std::string_view importPath)
{
	string_view ext = importPath.substr(importPath.find_last_of('.') + 1);

    if (ext == ext_) return true;
    return false;
}

unique_ptr<u8[]> IConverter::load(string_view importPath)
{
    FILE* fp = nullptr;
	errno_t error;

	error = fopen_s(&fp, importPath.data(), "rb");

	if (error != 0) return nullptr;

	//ファイルの末尾へ移動して、サイズを計算
	fseek(fp, 0L, SEEK_END);
	fpos_t size;
	fgetpos(fp, &size);

	//ファイルの最初に移動
	fseek(fp, 0L, SEEK_SET);

	unique_ptr<u8[]> rtBuff = make_unique<u8[]>(size);

	//サイズ分のデータを読み込む
	fread(rtBuff.get(), 1, size, fp);
	fclose(fp);

	return rtBuff;
}

u32 IConverter::write(string_view exportPath, u8 *data, const u32 dataSize)
{
    FILE* fp = nullptr;
	errno_t error;

	error = fopen_s(&fp, exportPath.data(), "wb");

	if (error != 0) return ERROR_FILE_OPERATION;

	fwrite(data, sizeof(u8), dataSize, fp);
	fclose(fp);

	return SUCCESS;
}

void Converter::addObserver(string ext, unique_ptr<IConverter> observer)
{
	observers_.emplace(ext, move(observer));
}
unique_ptr<FileData> Converter::fileAnalysis(string_view importPath)
{
	for (auto& observer : observers_)
	{
		if (observer.second->judgeExt(importPath))
		{
			unique_ptr<u8[]> importFileBuff = observer.second->load(importPath);
            if (importFileBuff == nullptr)
            {
                cout << "ファイルの読み込みに失敗しました。" << endl;
                return nullptr;
            }

			unique_ptr<FileData> fileData = observer.second->analysis(importFileBuff);
			if (fileData == nullptr)
			{
				cout << "ファイルの解析に失敗しました。" << endl;
				return nullptr;
			}

			return fileData;
		}
	}

	cout << "解析できるファイル形式が見つかりませんでした。" << endl;
	return nullptr;
}

u32 Converter::fileConvert(string_view exportPath, unique_ptr<FileData> &fileData)
{
	for (auto& observer : observers_)
    {
		if (observer.second->judgeExt(exportPath))
		{
			u32 dataSize = 0;
			unique_ptr<u8[]> exportBuff = observer.second->convert(fileData, dataSize);
			if (exportBuff == nullptr)
			{
				cout << "ファイルの変換に失敗しました。" << endl;
				return ERROR_CONVERSION_FAILED;
			}

			u32 result = observer.second->write(exportPath, exportBuff.get(), dataSize);
			if (result != SUCCESS)
			{
				cout << "ファイルの書き出しに失敗しました。" << endl;
				return result;
			}

			return result;
		}
	}

	cout << "変換できるファイル形式が見つかりませんでした。" << endl;
	return ERROR_FILE_OPERATION;
}