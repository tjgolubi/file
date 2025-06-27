/// @file File.cpp
/// @brief Implementation of the `tjg::File` class.
///
/// This source file provides the implementation of selected member
/// functions declared in File.h, including `error_string()` and
/// `ModeStr()`, which support detailed exception reporting and
/// translation of C++ stream modes to C-style fopen modes.
///
/// @see File.h for class definition and documentation.
/// @date 2025
/// @copyright
///   Copyright 2025 Terry Golubiewski. All rights reserved.
///   Distributed under the MIT License.

#include "File.h"

namespace tjg {

std::string File::error_string(str_arg what) const {
  const auto& name = _name.generic_string();
  auto result = std::string{};
  result.reserve(name.length() + traits_t::length(what) + 8);
  result.append("File ");
  result.append(name);
  result.append(": ");
  result.append(what);
  return result;
} // error_string

File::czstring File::ModeStr(std::ios_base::openmode mode) {

  static constexpr std::array<czstring, 8> TextModeStrings =
    { "r", "r+", "w", "w+", "wx", "w+x", "a", "a+" };

  static constexpr std::array<czstring, 8> BinaryModeStrings =
    { "rb", "r+b", "wb", "w+b", "wbx", "w+bx", "ab", "a+b" };

  constexpr auto in        = std::ios_base::in;
  constexpr auto out       = std::ios_base::out;
  constexpr auto trunc     = std::ios_base::trunc;
  constexpr auto app       = std::ios_base::app;
  constexpr auto noreplace = std::ios_base::noreplace;
  constexpr auto binary    = std::ios_base::binary;

  int index = 0;
  switch (mode & (in | out | trunc | app | noreplace)) {
    case in:           index = 0; break;
    case in|out:       index = 1; break;
    case out:          index = 2; break;
    case out|trunc:    index = 2; break;
    case in|out|trunc: index = 3; break;
    case out|noreplace:          index = 4; break;
    case out|trunc|noreplace:    index = 4; break;
    case in|out|trunc|noreplace: index = 5; break;
    case app:          index = 6; break;
    case app|out:      index = 6; break;
    case app|in:       index = 7; break;
    case app|in|out:   index = 7; break;
    default: return nullptr;
  }

  return (mode & binary) ?  BinaryModeStrings[index] : TextModeStrings[index];
} // ModeStr

} // tjg
