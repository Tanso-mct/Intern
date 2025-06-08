//------------------------------------------------------------------------------
//! @file	pch.h
//! @brief  プリコンパイルヘッダー
//! @author	yasunari shiino
//------------------------------------------------------------------------------
#pragma once

#include "type.h"

#include <Windows.h>

#include <fstream>
#include <cstdio>
#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <cstdint>
#include <cmath>
#include <vector>

// #define NDEBUG
#include <cassert>

/**
 * @brief main関数の戻り値。成功：0、失敗：1以上。エラーの種類によって異なる値を返す。
 */
constexpr u32 SUCCESS = 0;
constexpr u32 ERROR_INVALID_ARGUMENTS = 1;
constexpr u32 ERROR_FILE_OPERATION = 2;
constexpr u32 ERROR_FILE_LOAD_FAILED = 3;
constexpr u32 ERROR_CONVERSION_FAILED = 4;
