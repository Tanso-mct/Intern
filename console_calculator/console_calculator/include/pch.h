#pragma once

#include <iostream>
#include <memory>
#include <cstdlib>
#include <string>
#include <cctype>
#include <vector>
#include <unordered_map>
#include <stack>
#include <queue>
#include <utility>

#include "type.h"

constexpr int RESULT_SUCCESS = 0;
constexpr int RESULT_FAIL_TO_EXECUTE_CMD = 1;

constexpr const char CMD_NUMBER = 'n';
constexpr const char CMD_UNDO = 'u';
constexpr const char CMD_REDO = 'r';
constexpr const char CMD_EXECUTE = '=';

constexpr const int INVALID_PRIORITY = -1;

template <typename T, typename S>
T* PtrAs(S* source)
{
    T* target = dynamic_cast<T*>(source);
    return target;
}