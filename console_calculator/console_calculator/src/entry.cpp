#include "pch.h"

#include "calculator.h"
#include "command.h"

int main()
{
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    std::unordered_map<char, std::unique_ptr<Command>> cmdTable;
    cmdTable[CMD_NUMBER] = std::make_unique<NumberCmd>();
    cmdTable['+'] = std::make_unique<AddCmd>();
    cmdTable['-'] = std::make_unique<SubtractCmd>();
    cmdTable['*'] = std::make_unique<MultiplyCmd>();
    cmdTable['/'] = std::make_unique<DivideCmd>();
    cmdTable['('] = std::make_unique<LeftParenCmd>();
    cmdTable[')'] = std::make_unique<RightParenCmd>();

    bool result = true;
    std::string input;

    while (true)
    {
        calculator->show();
        if (calculator->getError()) return RESULT_FAIL_TO_EXECUTE_CMD;

        std::cin >> input;
        if (std::isdigit(input[0]) || (input[0] == '-' && input.size() > 1)) // 数値の場合
        {
            NumberCmd* numCmd = PtrAs<NumberCmd>(cmdTable[CMD_NUMBER].get());
            if (numCmd == nullptr) return RESULT_FAIL_TO_EXECUTE_CMD;
            numCmd->setNum(std::stod(input));

            calculator->appendCmd(cmdTable[CMD_NUMBER]);
        }
        else // 数値以外の場合
        {
            if (input[0] == CMD_UNDO)
            {
                calculator->undo();
                continue;
            }
            else if (input[0] == CMD_REDO)
            {
                calculator->redo();
                continue;
            }
            else if (input[0] == CMD_EXECUTE)
            {
                calculator->execute();
                continue;
            }

            // 登録されているか確認
            if (cmdTable.find(input[0]) == cmdTable.end())
            {
                calculator->setError("Error : Command not found");
                continue;
            }

            calculator->appendCmd(cmdTable[input[0]]);
        }
    }

    return RESULT_SUCCESS;
}