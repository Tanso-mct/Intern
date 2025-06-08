#include "pch.h"

#include "calculator.h"
#include "command.h"

std::queue<std::unique_ptr<Command>> RPN::ToRPN
(
    std::vector<std::unique_ptr<Command>>::iterator start, 
    std::vector<std::unique_ptr<Command>>::iterator end
){
    std::queue<std::unique_ptr<Command>> rpnCmds; // 逆ポーランド記法のコマンド列
    std::stack<std::unique_ptr<Command>> opeStack; // 演算子スタック

    std::vector<std::unique_ptr<Command>>::iterator it = start;
    int inParenDepth = 0;
    while (it != end)
    {
        if (PtrAs<NumberCmd>(it->get())) rpnCmds.emplace(std::move(*it)); // 数値の場合、キューにプッシュ
        else // 数値以外の場合
        {
            // 左括弧
            if (PtrAs<LeftParenCmd>(it->get()))
            {
                inParenDepth++;
                opeStack.emplace(std::move(*it));
                it++;
                continue;
            }

            if (opeStack.empty()) opeStack.emplace(std::move(*it));
            else
            {
                // 右括弧
                if (PtrAs<RightParenCmd>(it->get()))
                {
                    inParenDepth--;

                    // 左括弧までスタックから取り出す
                    while (!opeStack.empty() && !PtrAs<LeftParenCmd>(opeStack.top().get()))
                    {
                        rpnCmds.emplace(std::move(opeStack.top()));
                        opeStack.pop();
                    }

                    opeStack.pop(); // 左括弧を取り除く
                    it++;

                    continue;
                }

                // 括弧内の演算子の場合、スタックにプッシュ
                if (it->get()->priority() > opeStack.top()->priority()) opeStack.emplace(std::move(*it));
                else
                {
                    while (!opeStack.empty() && it->get()->priority() <= opeStack.top()->priority())
                    {
                        rpnCmds.emplace(std::move(opeStack.top()));
                        opeStack.pop();
                    }

                    opeStack.emplace(std::move(*it));
                }
            }
        }

        it++;
    }

    while (!opeStack.empty())
    {
        rpnCmds.emplace(std::move(opeStack.top()));
        opeStack.pop();
    }

    return rpnCmds;
}

std::unique_ptr<Command> RPN::CalcFromRPN
(
    Calculator* calculator,
    std::queue<std::unique_ptr<Command>> rpnCmds, std::unique_ptr<Command> srcNumCmd
){
    if (rpnCmds.size() < 3) return nullptr;
    if (!PtrAs<NumberCmd>(rpnCmds.front().get())) return nullptr;

    // スタックにキューからコマンドを取得しながら計算
    std::stack<std::unique_ptr<Command>> calcStack;

    while(!rpnCmds.empty()) // キューが空になるまで繰り返す
    {
        while(PtrAs<NumberCmd>(rpnCmds.front().get())) // キューの最前が数値である限りスタックにプッシュ
        {
            calcStack.emplace(std::move(rpnCmds.front()));
            rpnCmds.pop();
        }

        std::pair<double, bool> rightNum = calcStack.top()->execute(calculator, 0, 0);
        calcStack.pop();

        std::pair<double, bool> leftNum = calcStack.top()->execute(calculator, 0, 0);
        calcStack.pop();

        std::unique_ptr<Command> ope = std::move(rpnCmds.front());
        rpnCmds.pop();

        std::pair<double, bool> result = ope->execute(calculator, leftNum.first, rightNum.first);
        if (!result.second) return nullptr;

        // 結果の所有権を保持するため、resultsに結果を追加
        PtrAs<NumberCmd>(srcNumCmd.get())->setNum(result.first);

        // resultであるNumberCmdをスタックにプッシュ
        calcStack.emplace(srcNumCmd->clone());
    }

    return std::move(calcStack.top());
}

Calculator::Calculator()
{
    std::unique_ptr<Command> numCmd = std::make_unique<NumberCmd>();
    PtrAs<NumberCmd>(numCmd.get())->setNum(idleNum_);

    history_.emplace_back(std::move(numCmd)); // 初期数値を履歴に追加
    historyIt_ = history_.end() - 1;

    cmds_.emplace_back(historyIt_->get()->clone());
}

void Calculator::appendCmd(std::unique_ptr<Command>& cmd)
{
    bool appended = cmd->append(this, cmds_);
    if (appended)
    {
        if (PtrAs<LeftParenCmd>(cmd.get())) inParenDepth++;
        else if (PtrAs<RightParenCmd>(cmd.get())) inParenDepth--;

        // 履歴を更新
        history_.erase(historyIt_ + 1, history_.end()); // イテレーターより後ろの履歴を削除
        history_.emplace_back(cmd->clone()); // 履歴に追加
        historyIt_ = history_.end() - 1; // イテレーターを履歴の最後に
    }
}

bool Calculator::getError()
{
    return error_;
}

void Calculator::setError(const std::string &msg)
{
    cmdsOutput_ = msg;
    error_ = true;
}

void Calculator::show()
{
    if (!error_)
    {
        cmdsOutput_ = "";
        for (const auto& cmd : cmds_) cmdsOutput_ += cmd->toString() + " ";
    }

    system("cls");
    std::cout << "---------------------------------------------------------------------------------------" << std::endl;
    std::cout << "Console Calculator" << std::endl;
    std::cout << "---------------------------------------------------------------------------------------" << std::endl;
    std::cout << "Please enter one number or operator at a time." << std::endl;
    std::cout << "Only '+', '-', '*', '/', '(', ')' or '=' operators are supported." << std::endl;
    std::cout << "Enter 'u' to undo, 'r' to redo." << std::endl;
    std::cout << "---------------------------------------------------------------------------------------" << std::endl;
    std::cout << cmdsOutput_ << std::endl;
    std::cout << "---------------------------------------------------------------------------------------" << std::endl;
}

bool Calculator::appendNumberCmd(std::vector<std::unique_ptr<Command>> &dst, std::unique_ptr<Command> src)
{
    if (dst.empty()) return false;

    if (dst.size() == 1 && PtrAs<NumberCmd>(dst[0].get())) // 上書き
    {
        dst.clear();
        dst.emplace_back(std::move(src));
        return true;
    }
    else if (!PtrAs<NumberCmd>(dst.back().get()) && !PtrAs<RightParenCmd>(dst.back().get()))
    {
        dst.emplace_back(std::move(src));
        return true;
    }

    return false;
}

std::pair<double, bool> Calculator::add(double leftNum, double rightNum)
{
    return std::make_pair(leftNum + rightNum, true);
}

std::pair<double, bool> Calculator::subtract(double leftNum, double rightNum)
{
    return std::make_pair(leftNum - rightNum, true);
}

std::pair<double, bool> Calculator::multiply(double leftNum, double rightNum)
{
    return std::make_pair(leftNum * rightNum, true);
}

std::pair<double, bool> Calculator::divide(double leftNum, double rightNum)
{
    if (rightNum == 0.0)
    {
        cmdsOutput_ = "Error : Division by zero";
        error_ = true;
        return std::make_pair(0.0, false);
    }

    return std::make_pair(leftNum / rightNum, true);
}

bool Calculator::appendOpeCmd(std::vector<std::unique_ptr<Command>> &dst, std::unique_ptr<Command> src)
{
    if (dst.empty()) return false;

    if (dst.size() == 1 && PtrAs<NumberCmd>(dst[0].get()))
    {
        dst.emplace_back(std::move(src));
        return true;
    }
    else if (PtrAs<NumberCmd>(dst.back().get()))
    {
        dst.emplace_back(std::move(src));
        return true;
    }
    else if (PtrAs<RightParenCmd>(dst.back().get()))
    {
        dst.emplace_back(std::move(src));
        return true;
    }

    return false;
}

bool Calculator::appendLeftParenCmd(std::vector<std::unique_ptr<Command>> &dst, std::unique_ptr<Command> src)
{
    if (dst.empty()) return false;

    if (dst.size() == 1 && PtrAs<NumberCmd>(dst[0].get())) // 上書き
    {
        dst.clear();
        dst.emplace_back(std::move(src));
        return true;
    }
    else if (!PtrAs<NumberCmd>(dst.back().get()) && !PtrAs<RightParenCmd>(dst.back().get()))
    {
        dst.emplace_back(std::move(src));
        return true;
    }

    return false;
}

bool Calculator::appendRightParenCmd(std::vector<std::unique_ptr<Command>> &dst, std::unique_ptr<Command> src)
{
    if (dst.empty()) return false;

    if (PtrAs<NumberCmd>(dst.back().get()) || PtrAs<RightParenCmd>(dst.back().get()))
    {
        dst.emplace_back(std::move(src));
        return true;
    }

    return false;
}

void Calculator::execute()
{
    if (cmds_.size() >= 3) // 計算が行える場合、計算を行う
    {
        if (inParenDepth != 0) // 括弧が閉じられていない場合
        {
            cmdsOutput_ = "Error : Parentheses are not closed";
            error_ = true;
            return;
        }

        // 逆ポーランド記法に変換
        std::queue<std::unique_ptr<Command>> rpnCmds = RPN::ToRPN(cmds_.begin(), cmds_.end());

        // // 逆ポーランド記法から計算
        std::unique_ptr<Command> result = RPN::CalcFromRPN
        (
            this, std::move(rpnCmds), std::make_unique<NumberCmd>()
        );

        if (result == nullptr || error_) return; // 計算に失敗した場合

        // 計算結果を追加
        cmds_.clear();
        cmds_.emplace_back(std::move(result));

        // 履歴を結果から初期化
        history_.clear();

        std::unique_ptr<Command> numCmd = std::make_unique<NumberCmd>();
        PtrAs<NumberCmd>(numCmd.get())->setNum(idleNum_);

        history_.emplace_back(std::move(numCmd)); // 初期数値を履歴に追加
        history_.emplace_back(cmds_.back().get()->clone()); // 結果を履歴に追加

        historyIt_ =  history_.end() - 1; // イテレーターを履歴の最後に
    }
}

void Calculator::undo()
{
    if (cmds_.size() == 1) // これ以上戻れない場合
    {
        cmds_.pop_back();
        std::unique_ptr<Command> numCmd = std::make_unique<NumberCmd>();
        PtrAs<NumberCmd>(numCmd.get())->setNum(idleNum_);

        cmds_.emplace_back(std::move(numCmd));

        historyIt_ = history_.begin();
        return;
    }

    cmds_.pop_back();
    historyIt_--;
}

void Calculator::redo()
{
    if (historyIt_ == history_.end() - 1) return; // これ以上進めない場合

    historyIt_++;
    historyIt_->get()->append(this, cmds_);
}
