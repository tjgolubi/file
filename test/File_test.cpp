
/// @file File_test.cpp
/// @brief GoogleTest Testing for File.h.
///
/// @author Terry Golubiewski
/// @date 2025
/// @copyright
///   Copyright 2025 Terry Golubiewski. All rights reserved.
///   Distributed under the MIT License.

#include "File.h"

#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <vector>
#include <array>

namespace fs = std::filesystem;
using tjg::File;

class FileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tmpfile_path = fs::temp_directory_path() / "gtest_tempfile.txt";
    auto f = std::fopen(tmpfile_path.string().c_str(), "w");
    if (!!f) {
      std::fputs("line 1\nline 2\nline 3\n", f);
      std::fclose(f);
    }
  }

  void TearDown() override {
    auto ec = std::error_code{};
    fs::remove(tmpfile_path, ec);
  }

  fs::path tmpfile_path;

  static std::string read_line(File& f) {
    auto buf = File::Buffer{};
    f.gets(buf);
    return std::string{buf.data()};
  }
}; // FileTest

TEST_F(FileTest, OpenClose) {
  auto f = File{tmpfile_path, std::ios_base::in};
  EXPECT_TRUE(f.is_open());
  f.close();
  EXPECT_FALSE(f.is_open());
} // OpenClose

TEST_F(FileTest, ReadLinesUsingGets) {
  auto f = File{tmpfile_path, std::ios_base::in};
  auto expected = std::vector<std::string>{"line 1\n", "line 2\n", "line 3\n"};
  auto buf = File::Buffer{};
  for (const auto& exp : expected) {
    EXPECT_TRUE(f.gets(buf));
    EXPECT_EQ(std::string{buf.data()}, exp);
  }
  EXPECT_FALSE(f.gets(buf));
  EXPECT_TRUE(f.eof());
} // ReadLineUsingGets

TEST_F(FileTest, CtorThrowsOnMissingFile) {
  auto badpath = fs::path{tmpfile_path};
  badpath += ".doesnotexist";
  auto doit = [badpath]() { auto f = File{badpath, std::ios_base::in}; };
  EXPECT_THROW({ doit(); }, std::logic_error);
} // CtorThrowsOnMissingFile

TEST_F(FileTest, WriteAndReadBack) {
  auto wpath = tmpfile_path.parent_path() / "gtest_tempout.txt";
  {
    auto f = File{wpath, std::ios_base::out};
    f.puts("hello world\n");
  }
  {
    auto f = File{wpath, std::ios_base::in};
    auto buf = File::Buffer{};
    EXPECT_TRUE(f.gets(buf));
    EXPECT_EQ(std::string{buf.data()}, "hello world\n");
    EXPECT_FALSE(f.gets(buf));
    EXPECT_TRUE(f.eof());
  }
  std::error_code ec;
  fs::remove(wpath, ec);
} // WriteAndReadBack

TEST_F(FileTest, MoveCtorTransfersOwnership) {
  auto f1 = File{tmpfile_path, std::ios_base::in};
  auto f2 = std::move(f1);
  EXPECT_FALSE(f1.is_open());  // source should be invalidated
  EXPECT_TRUE(f2.is_open());
} // MoveCtorTransfersOwnership

TEST_F(FileTest, MoveAssignmentTransfersOwnership) {
  File f1{tmpfile_path, std::ios_base::in};
  File f2 = std::move(f1);
  EXPECT_FALSE(f1.is_open());
  EXPECT_TRUE(f2.is_open());

  File f3;
  f3 = std::move(f2);
  EXPECT_FALSE(f2.is_open());
  EXPECT_TRUE(f3.is_open());
} // MoveAssignmentTransfersOwnership
