#include "pch.h"

#include "converter.h"

#include "format_bmp.h"
#include "format_tga.h"
#include "format_dds.h"

using namespace std;

namespace
{

string WideCharToMultiByteString(const wstring& wstr) 
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

}

int main(int argc, char* argv[])
{
    // 引数の数が合わない場合、エラーを出力して終了
    if (argc != 5)
    {
        cout << "引数の数が合いません。以下の例のように実行してください。" << endl;
        cout << "image_format_converter.exe /i ファイルパス /o 出力フォルダ" << endl;

        return ERROR_INVALID_ARGUMENTS;
    }

    // /i、/oがそれぞれ一つずつ指定されているか確認
    int isArgCorrect = 1;
    isArgCorrect *= (string(argv[1]) == "/i" && string(argv[3]) == "/i") ? 0 : 1;
    isArgCorrect *= (string(argv[1]) == "/o" && string(argv[3]) == "/o") ? 0 : 1;

    if (isArgCorrect != 1)
    {
        cout << "引数が不正です。/i、/oを使用し、入力ファイル、出力フォルダを指定してください。" << endl;
        return ERROR_INVALID_ARGUMENTS;
    }

    string importPath;
    string exportPath;
    for (int i = 1; i <= 3; i += 2)
    {
        assert(i <= 3);

        if (string(argv[i]) == "/i") importPath = argv[i+1]; // 入力ファイルパスを取得
        else if (string(argv[i]) == "/o") exportPath = argv[i+1]; // 出力フォルダパスを取得
    }


    // 引数が正しく取得できているか確認
    if (importPath.empty() || exportPath.empty())
    {
        cout << "読み込めるファイル形式が見つかりませんでした。" << endl;
        return ERROR_FILE_LOAD_FAILED;
    }

    // 変換Subjectに変換クラスを登録
    Converter converter;
    converter.addObserver("bmp", make_unique<BMP>());
    converter.addObserver("tga", make_unique<TGA>(true)); // 圧縮を使用する
    converter.addObserver("dds", make_unique<DDS>());

    // ファイルの読み込み、解析を行い、ファイルデータを取得
    unique_ptr<FileData> fileData = converter.fileAnalysis(importPath);
    if (fileData == nullptr) return ERROR_FILE_OPERATION;

    // ファイルを変換し、書き出す
    u32 result = converter.fileConvert(exportPath, fileData);
    if (result != SUCCESS) return result;
}