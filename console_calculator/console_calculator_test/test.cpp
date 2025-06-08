#include "pch.h"

#include "console_calculator/include/pch.h"
#include "console_calculator/include/calculator.h"
#include "console_calculator/include/command.h"

TEST(CalculatorTest, Addition) 
{
    // RPN 1 2 +
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {1.0, 1.0};
    
    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 1

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 1

    cmds.emplace(std::make_unique<AddCmd>()); // +

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    EXPECT_DOUBLE_EQ
    (
        2, 
        PtrAs<NumberCmd>(RPN::CalcFromRPN
        (
            calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
        ).get())->getNum()
    );
}

TEST(CalculatorTest, Subtraction) 
{
    // RPN 1 2 -
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {1.0, 2.0};

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 1    

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 2

    cmds.emplace(std::make_unique<SubtractCmd>()); // -

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    EXPECT_DOUBLE_EQ
    (
        -1.0, 
        PtrAs<NumberCmd>(RPN::CalcFromRPN
        (
            calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
        ).get())->getNum()
    );
}

TEST(CalculatorTest, Multiplication) 
{
    // RPN 2 3 *
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {2.0, 3.0};

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 2

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 3

    cmds.emplace(std::make_unique<MultiplyCmd>()); // *

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    EXPECT_DOUBLE_EQ
    (
        6,
        PtrAs<NumberCmd>(RPN::CalcFromRPN
        (
            calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
        ).get())->getNum()
    );
}

TEST(CalculatorTest, Division) 
{
    // RPN 6 3 /
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {6.0, 3.0};

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 6

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 3

    cmds.emplace(std::make_unique<DivideCmd>()); // /

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    EXPECT_DOUBLE_EQ
    (
        2,
        PtrAs<NumberCmd>(RPN::CalcFromRPN
        (
            calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
        ).get())->getNum()
    );
}

TEST(CalculatorTest, RPNConvert)
{
    // 1 + 2 * 3
    std::vector<std::unique_ptr<Command>> cmds;
    double nums[] = {1.0, 2.0, 3.0};

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 1

    cmds.emplace_back(std::make_unique<AddCmd>()); // +

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 2

    cmds.emplace_back(std::make_unique<MultiplyCmd>()); // *

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[2]); // 3

    // 逆ポーランド記法に変換
    std::queue<std::unique_ptr<Command>> rpnCmds = RPN::ToRPN(cmds.begin(), cmds.end());

    // 結果の文字列
    std::string result;
    while (!rpnCmds.empty())
    {
        result += rpnCmds.front()->toString();
        rpnCmds.pop();
    }

    // 123*+
    std::string expect = std::to_string(nums[0]) + std::to_string(nums[1]) + std::to_string(nums[2]) + "*+";

    EXPECT_EQ(expect, result);
}

TEST(CalculatorTest, RPNConvertParentheses)
{
    // (1 + 3 * (2 + -3)) * 3 - 10 / 2
    std::vector<std::unique_ptr<Command>> cmds;
    double nums[] = {1.0, 3.0, 2.0, -3.0, 3.0, 10.0, 2.0};

    cmds.emplace_back(std::make_unique<LeftParenCmd>()); // (

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 1

    cmds.emplace_back(std::make_unique<AddCmd>()); // +

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 3

    cmds.emplace_back(std::make_unique<MultiplyCmd>()); // *

    cmds.emplace_back(std::make_unique<LeftParenCmd>()); // (

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[2]); // 2

    cmds.emplace_back(std::make_unique<AddCmd>()); // +

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[3]); // -3

    cmds.emplace_back(std::make_unique<RightParenCmd>()); // )

    cmds.emplace_back(std::make_unique<RightParenCmd>()); // )

    cmds.emplace_back(std::make_unique<MultiplyCmd>()); // *

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[4]); // 3

    cmds.emplace_back(std::make_unique<SubtractCmd>()); // -

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[5]); // 10

    cmds.emplace_back(std::make_unique<DivideCmd>()); // /

    cmds.emplace_back(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[6]); // 2

    // 逆ポーランド記法に変換
    std::queue<std::unique_ptr<Command>> rpnCmds = RPN::ToRPN(cmds.begin(), cmds.end());

    // 結果の文字列
    std::string result;
    while (!rpnCmds.empty())
    {
        result += rpnCmds.front()->toString();
        rpnCmds.pop();
    }

    //1 3 2 -3 + * + 3 * 10 2 / -
    std::string expect 
    = std::to_string(nums[0]) + std::to_string(nums[1]) + std::to_string(nums[2]) + std::to_string(nums[3]) + "+*+" 
    + std::to_string(nums[4]) + "*" + std::to_string(nums[5]) + std::to_string(nums[6]) + "/-";

    EXPECT_EQ(expect, result);
}

TEST(CalculatorTest, CalculationOrder)
{
    // 1 + 2 * 3 - -4 / 2
    // RPN 1 2 3 * + -4 2 / -
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {1.0, 2.0, 3.0, -4.0, 2.0};

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 1

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 2

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[2]); // 3

    cmds.emplace(std::make_unique<MultiplyCmd>()); // *

    cmds.emplace(std::make_unique<AddCmd>()); // +

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[3]); // -4

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[4]); // 2

    cmds.emplace(std::make_unique<DivideCmd>()); // /

    cmds.emplace(std::make_unique<SubtractCmd>()); // -
    
    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    EXPECT_DOUBLE_EQ
    (
        9,
        PtrAs<NumberCmd>(RPN::CalcFromRPN
        (
            calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
        ).get())->getNum()
    );
}

TEST(CalculatorTest, ZeroDivision)
{
    // 1 / 0
    // RPN 1 0 /
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {1.0, 0.0};

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 1

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 0

    cmds.emplace(std::make_unique<DivideCmd>()); // /

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    std::unique_ptr<Command> result = RPN::CalcFromRPN
    (
        calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
    );

    EXPECT_FALSE(result);
}

TEST(CalculatorTest, StartIsNotNum)
{
    // (1 + 2
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {1.0, 2.0};

    cmds.emplace(std::make_unique<LeftParenCmd>()); // (

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 1

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 2

    cmds.emplace(std::make_unique<AddCmd>()); // +

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    std::unique_ptr<Command> result =  RPN::CalcFromRPN
    (
        calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
    );

    EXPECT_FALSE(result);
}

TEST(CalculatorTest, DecimalCalculation)
{
    // RPN 1 0.9 -
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {1, 0.9};

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 1

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 0.9

    cmds.emplace(std::make_unique<SubtractCmd>()); // 0.1

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    EXPECT_DOUBLE_EQ
    (
        0.1,
        PtrAs<NumberCmd>(RPN::CalcFromRPN
        (
            calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
        ).get())->getNum()
    );
}

TEST(CalculatorTest, UndividedDivision)
{
    // RPN 1 3 /
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {1.0, 3.0};

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 1

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 3

    cmds.emplace(std::make_unique<DivideCmd>()); // /

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    EXPECT_DOUBLE_EQ
    (
        0.3333333333333333, // doubleは15桁程度の精度のため16桁に
        PtrAs<NumberCmd>(RPN::CalcFromRPN
        (
            calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
        ).get())->getNum()
    );
}

TEST(CalculatorTest, EqualZero)
{
    // RPN 2.5 2.5 -
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {2.5, 2.5};

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 2.5

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 2.5

    cmds.emplace(std::make_unique<SubtractCmd>()); // -

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    EXPECT_DOUBLE_EQ
    (
        0,
        PtrAs<NumberCmd>(RPN::CalcFromRPN
        (
            calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
        ).get())->getNum()
    );
}

TEST(CalculatorTest, EqualMinus)
{
    // RPN 2.5 11.7 -
    std::queue<std::unique_ptr<Command>> cmds;
    double nums[] = {2.5, 11.7};

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[0]); // 2.5

    cmds.emplace(std::make_unique<NumberCmd>());
    PtrAs<NumberCmd>(cmds.back().get())->setNum(nums[1]); // 11.7

    cmds.emplace(std::make_unique<SubtractCmd>()); // -

    // 計算に使用しなければいけないインスタンスの生成
    std::unique_ptr<Calculator> calculator = std::make_unique<Calculator>();

    // 逆ポーランド記法から計算
    EXPECT_DOUBLE_EQ
    (
        -9.2,
        PtrAs<NumberCmd>(RPN::CalcFromRPN
        (
            calculator.get(), std::move(cmds), std::make_unique<NumberCmd>()
        ).get())->getNum()
    );
}

int main(int argc, char **argv) 
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}