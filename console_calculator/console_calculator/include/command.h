#pragma once

#include <memory>
#include <utility>

class Calculator;

class Command
{
public:
    virtual ~Command() = default;
    virtual std::pair<double, bool> execute(Calculator* calculator, double leftNum, double rightNum) = 0;
    virtual std::string toString() const = 0;
    virtual std::unique_ptr<Command> clone() = 0;
    virtual int priority() const { return 0; }
    virtual bool append(Calculator* calculator, std::vector<std::unique_ptr<Command>>& dst) = 0;
};

class NumberCmd : public Command
{
private:
    double num_ = 0.0;

public:
    NumberCmd() = default;
    ~NumberCmd() override = default;

    std::pair<double, bool> execute(Calculator* calculator, double leftNum, double rightNum) override;

    std::string toString() const override { return std::to_string(num_); }
    std::unique_ptr<Command> clone() override { return std::make_unique<NumberCmd>(*this); }
    int priority() const override { return 0; }

    virtual bool append(Calculator* calculator, std::vector<std::unique_ptr<Command>>& dst) override;

    void setNum(double num) { num_ = num; }
    double getNum() const { return num_; }
};

class AddCmd : public Command
{
public:
    AddCmd() = default;
    ~AddCmd() override = default;
    
    std::pair<double, bool> execute(Calculator* calculator, double leftNum, double rightNum) override;

    std::string toString() const override { return "+"; }
    std::unique_ptr<Command> clone() override { return std::make_unique<AddCmd>(*this); }
    int priority() const override { return 1; }

    virtual bool append(Calculator* calculator, std::vector<std::unique_ptr<Command>>& dst) override;
};

class SubtractCmd : public Command
{
public:
    SubtractCmd() = default;
    ~SubtractCmd() override = default;

    std::pair<double, bool> execute(Calculator* calculator, double leftNum, double rightNum) override;

    std::string toString() const override { return "-"; }
    std::unique_ptr<Command> clone() override { return std::make_unique<SubtractCmd>(*this); }
    int priority() const override { return 1; }

    virtual bool append(Calculator* calculator, std::vector<std::unique_ptr<Command>>& dst) override;
};

class MultiplyCmd : public Command
{
public:
    MultiplyCmd() = default;
    ~MultiplyCmd() override = default;

    std::pair<double, bool> execute(Calculator* calculator, double leftNum, double rightNum) override;

    std::string toString() const override { return "*"; }
    std::unique_ptr<Command> clone() override { return std::make_unique<MultiplyCmd>(*this); }
    int priority() const override { return 2; }

    virtual bool append(Calculator* calculator, std::vector<std::unique_ptr<Command>>& dst) override;
};

class DivideCmd : public Command
{
public:
    DivideCmd() = default;
    ~DivideCmd() override = default;

    std::pair<double, bool> execute(Calculator* calculator, double leftNum, double rightNum) override;

    std::string toString() const override { return "/"; }
    std::unique_ptr<Command> clone() override { return std::make_unique<DivideCmd>(*this); }
    int priority() const override { return 2; }

    virtual bool append(Calculator* calculator, std::vector<std::unique_ptr<Command>>& dst) override;
};

class LeftParenCmd : public Command
{
public:
    LeftParenCmd() = default;
    ~LeftParenCmd() override = default;

    std::pair<double, bool> execute(Calculator* calculator, double leftNum, double rightNum) override;

    std::string toString() const override { return "("; }
    std::unique_ptr<Command> clone() override { return std::make_unique<LeftParenCmd>(*this); }
    int priority() const override { return 0; }

    virtual bool append(Calculator* calculator, std::vector<std::unique_ptr<Command>>& dst) override;
};

class RightParenCmd : public Command
{
public:
    RightParenCmd() = default;
    ~RightParenCmd() override = default;

    std::pair<double, bool> execute(Calculator* calculator, double leftNum, double rightNum) override;

    std::string toString() const override { return ")"; }
    std::unique_ptr<Command> clone() override { return std::make_unique<RightParenCmd>(*this); }
    int priority() const override { return 0; }
    
    virtual bool append(Calculator* calculator, std::vector<std::unique_ptr<Command>>& dst) override;
};