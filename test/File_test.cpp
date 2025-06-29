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

#include <filesystem>
#include <string>
#include <vector>
#include <array>
#include <stdexcept>
#include <cstdio>

#include <iostream>

namespace fs = std::filesystem;
using namespace std::string_literals;
using tjg::File;

class FileTest : public ::testing::Test {
 protected:

  using Buffer = File::Buffer<6>;

  static const std::vector<std::string> ThreeLineVec;
  static const std::array<char, 22> ThreeLineStr;
  static const std::array<Buffer, 3> ThreeLineBuf;

  static const gsl::czstring HelloWorld;

  fs::path tmpfile_path;

  void SetUp() override {
    tmpfile_path = fs::temp_directory_path() / "gtest_tempfile.txt";
    auto f = std::fopen(tmpfile_path.string().c_str(), "w");
    if (!!f) {
      std::fputs(ThreeLineStr.data(), f);
      std::fclose(f);
    }
  }

  void TearDown() override {
    auto ec = std::error_code{};
    fs::remove(tmpfile_path, ec);
  }

}; // FileTest

const std::vector<std::string> FileTest::ThreeLineVec{
  "line 1\n"s,
  "line 2\n"s,
  "line 3\n"s
};

const std::array<char, 22> FileTest::ThreeLineStr = {
  "line 1\nline 2\nline 3\n"
};

const std::array<FileTest::Buffer, 3> FileTest::ThreeLineBuf  = {
  Buffer{ 'L', 'i', 'n', 'e', '1', '\n' },
  Buffer{ 'L', 'i', 'n', 'e', '2', '\n' },
  Buffer{ 'L', 'i', 'n', 'e', '3', '\n' }
};

const gsl::czstring FileTest::HelloWorld = "Hello, world!\n";

TEST_F(FileTest, OpenClose) {
  auto f = File{tmpfile_path, std::ios_base::in};
  EXPECT_TRUE(f.is_open());
  f.close();
  EXPECT_FALSE(f.is_open());
} // OpenClose

TEST_F(FileTest, ReadLinesUsingGets) {
  auto f = File{tmpfile_path, std::ios_base::in};
  const auto& expected = ThreeLineVec;
  auto buf = File::Buffer{};
  for (const auto& exp : expected) {
    EXPECT_TRUE(f.gets(buf));
    EXPECT_STREQ(buf.data(), exp.data());
  }
  EXPECT_FALSE(f.gets(buf));
  EXPECT_TRUE(f.eof());
} // ReadLineUsingGets

TEST_F(FileTest, CtorThrowsOnMissingFile) {
  auto badpath = fs::path{tmpfile_path};
  badpath += ".doesnotexist";
  auto doit = [badpath]() { auto f = File{badpath, std::ios_base::in}; };
  EXPECT_THROW({ doit(); }, std::system_error);
} // CtorThrowsOnMissingFile

TEST_F(FileTest, PutAndGetBack) {
  const auto wpath = tmpfile_path.parent_path() / "gtest_tempout.txt";
  {
    auto f = File{wpath, std::ios_base::out};
    f.puts(HelloWorld);
  }
  {
    auto f = File{wpath, std::ios_base::in};
    auto buf = File::Buffer{};
    EXPECT_TRUE(f.gets(buf));
    EXPECT_STREQ(buf.data(), HelloWorld);
    EXPECT_FALSE(f.gets(buf));
    EXPECT_TRUE(f.eof());
  }
  std::error_code ec;
  fs::remove(wpath, ec);
} // PutAndGetBack

TEST_F(FileTest, WriteAndReadBack) {
  const auto& a1 = ThreeLineBuf;
  const auto wpath = tmpfile_path.parent_path() / "gtest_tempout.txt";
  {
    auto f = File{wpath, std::ios_base::out};
    f.write(ThreeLineBuf);
  }
  {
    std::array<Buffer, 3> a2;
    EXPECT_NE(a1, a2); 
    auto f = File{wpath, std::ios_base::in};
    EXPECT_EQ(3, f.read(a2.data(), a2[0].size(), a2.size()));
    EXPECT_EQ(a1, a2); 
    EXPECT_EQ(0, f.read(a2.data(), a2[0].size(), a2.size()));
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

#if 0
TEST_F(FileTest, SeekAndTell) {
  File f1{tmpfile_path, std::ios_base::in | std::ios_base::out};
  EXPECT_TRUE(f1.is_open());
  f1.write
} // SeekAndTell
#endif

