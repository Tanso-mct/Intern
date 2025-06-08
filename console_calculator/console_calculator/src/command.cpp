#include "pch.h"

#include "command.h"
#include "calculator.h"

std::pair<double, bool> NumberCmd::execute(Calculator* calculator, double leftNum, double rightNum)
{
    return std::make_pair(num_, true);
}

bool NumberCmd::append(Calculator* calculator, std::vector<std::unique_ptr<Command>> &dst)
{
    return calculator->appendNumberCmd(dst, this->clone());
}

std::pair<double, bool> AddCmd::execute(Calculator* calculator, double leftNum, double rightNum)
{
    return calculator->add(leftNum, rightNum);
}

bool AddCmd::append(Calculator* calculator, std::vector<std::unique_ptr<Command>> &dst)
{
    return calculator->appendOpeCmd(dst, this->clone());
}

std::pair<double, bool> SubtractCmd::execute(Calculator* calculator, double leftNum, double rightNum)
{
    return calculator->subtract(leftNum, rightNum);
}

bool SubtractCmd::append(Calculator* calculator, std::vector<std::unique_ptr<Command>> &dst)
{
    return calculator->appendOpeCmd(dst, this->clone());
}

std::pair<double, bool> MultiplyCmd::execute(Calculator* calculator, double leftNum, double rightNum)
{
    return calculator->multiply(leftNum, rightNum);
}

bool MultiplyCmd::append(Calculator* calculator, std::vector<std::unique_ptr<Command>> &dst)
{
    return calculator->appendOpeCmd(dst, this->clone());
}

std::pair<double, bool> DivideCmd::execute(Calculator* calculator, double leftNum, double rightNum)
{
    return calculator->divide(leftNum, rightNum);
}

bool DivideCmd::append(Calculator* calculator, std::vector<std::unique_ptr<Command>> &dst)
{
    return calculator->appendOpeCmd(dst, this->clone());
}

std::pair<double, bool> LeftParenCmd::execute(Calculator* calculator, double leftNum, double rightNum)
{
    return std::make_pair(0.0, true);
}

bool LeftParenCmd::append(Calculator* calculator, std::vector<std::unique_ptr<Command>> &dst)
{
    return calculator->appendLeftParenCmd(dst, this->clone());
}

std::pair<double, bool> RightParenCmd::execute(Calculator* calculator, double leftNum, double rightNum)
{
    return std::make_pair(0.0, true);
}

bool RightParenCmd::append(Calculator* calculator, std::vector<std::unique_ptr<Command>> &dst)
{
    return calculator->appendRightParenCmd(dst, this->clone());
}
