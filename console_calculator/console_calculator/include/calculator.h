#pragma once

#include <memory>
#include <vector>
#include <string>
#include <queue>

class Command;
class NumberCmd;
class Calculator;

namespace RPN
{

std::queue<std::unique_ptr<Command>> ToRPN
(
    std::vector<std::unique_ptr<Command>>::iterator start,
    std::vector<std::unique_ptr<Command>>::iterator end
);

std::unique_ptr<Command> CalcFromRPN
(
    Calculator* calculator,
    std::queue<std::unique_ptr<Command>> rpnCmds, std::unique_ptr<Command> srcNumCmd
);

}

class Calculator
{
private:
    std::vector<std::unique_ptr<Command>> history_;
    std::vector<std::unique_ptr<Command>>::iterator historyIt_ = history_.end();
    double idleNum_ = 0.0;

    std::vector<std::unique_ptr<Command>> cmds_;
    std::string cmdsOutput_;

    int inParenDepth = 0;
    bool error_ = false;

public:
    Calculator();
    ~Calculator() = default;

    void appendCmd(std::unique_ptr<Command>& cmd);

    bool getError();
    void setError(const std::string& msg);
    void show();

    bool appendNumberCmd(std::vector<std::unique_ptr<Command>>& dst, std::unique_ptr<Command> src);

    std::pair<double, bool> add(double leftNum, double rightNum);
    std::pair<double, bool> subtract(double leftNum, double rightNum);
    std::pair<double, bool> multiply(double leftNum, double rightNum);
    std::pair<double, bool> divide(double leftNum, double rightNum);
    bool appendOpeCmd(std::vector<std::unique_ptr<Command>>& dst, std::unique_ptr<Command> src);

    bool appendLeftParenCmd(std::vector<std::unique_ptr<Command>>& dst, std::unique_ptr<Command> src);
    bool appendRightParenCmd(std::vector<std::unique_ptr<Command>>& dst, std::unique_ptr<Command> src);

    void execute();
    void undo();
    void redo();
};