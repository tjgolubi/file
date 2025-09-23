/// @file
/// GoogleTest Testing for File.hpp.
///
/// @author Terry Golubiewski
/// @copyright 2025 Terry Golubiewski, all rights reserved.

#include "File.hpp"

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
  static const std::array<char, 18> ThreeLineArr;
  static const std::array<Buffer, 3> ThreeLineBuf;
  static const gsl::czstring ThreeLineStr;

  static const gsl::czstring HelloWorld;

  fs::path tmpfile_path;

  void SetUp() override {
    tmpfile_path = fs::temp_directory_path() / "gtest_tempfile.txt";
    auto f = std::fopen(tmpfile_path.string().c_str(), "w");
    if (!!f) {
      std::fputs(ThreeLineStr, f);
      std::fclose(f);
    }
  }

  void TearDown() override {
    auto ec = std::error_code{};
    fs::remove(tmpfile_path, ec);
  }

}; // FileTest

const std::vector<std::string> FileTest::ThreeLineVec{
  "Line1\n"s,
  "Line2\n"s,
  "Line3\n"s
};

const gsl::czstring FileTest::ThreeLineStr = "Line1\nLine2\nLine3\n";

const std::array<FileTest::Buffer, 3> FileTest::ThreeLineBuf  = {
  Buffer{ 'L', 'i', 'n', 'e', '1', '\n' },
  Buffer{ 'L', 'i', 'n', 'e', '2', '\n' },
  Buffer{ 'L', 'i', 'n', 'e', '3', '\n' }
};

const std::array<char, 18> FileTest::ThreeLineArr = {
   'L', 'i', 'n', 'e', '1', '\n',
   'L', 'i', 'n', 'e', '2', '\n',
   'L', 'i', 'n', 'e', '3', '\n'
};

const gsl::czstring FileTest::HelloWorld = "Hello, world!\n";

TEST_F(FileTest, OpenClose) {
  std::FILE* fp = nullptr;
  auto f = File{tmpfile_path, std::ios_base::in};
  EXPECT_TRUE(f.is_open());
  fp = f;
  EXPECT_NE(fp, nullptr);
  f.close();
  EXPECT_FALSE(f.is_open());
  EXPECT_EQ(f.name(), tmpfile_path);
  fp = f;
  EXPECT_EQ(fp, nullptr);
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

TEST_F(FileTest, SeekAndTell) {
  File f{tmpfile_path,
         std::ios_base::in | std::ios_base::out | std::ios_base::trunc};
  EXPECT_TRUE(f.is_open());
  f.write(ThreeLineBuf);
  const long totalSize = sizeof(ThreeLineBuf);
  EXPECT_EQ(totalSize, f.tell());
  const long recSize = sizeof(Buffer);
  {
    f.rewind();
    File::Buffer buf;
    buf.fill('\0');
    auto len = f.read(buf);
    EXPECT_TRUE(f.eof());
    EXPECT_FALSE(f.error());
    EXPECT_EQ(len, sizeof(ThreeLineBuf));
    EXPECT_STREQ(buf.data(), ThreeLineStr);
  }
  f.seek(-recSize, File::Seek::Current);
  EXPECT_EQ(totalSize-recSize, f.tell());
  EXPECT_FALSE(f.eof());
  FileTest::Buffer buf;
  buf.fill('\0');
  {
    static const gsl::czstring exp = "Line3";
    EXPECT_TRUE(f.gets(buf));
    EXPECT_STREQ(buf.data(), exp);
  }
  f.seek(recSize, File::Seek::Set);
  EXPECT_EQ(recSize, f.tell());
  {
    std::array<Buffer, 3> buf;
    for (auto& b : buf)
      b.fill('\0');
    static const std::array<Buffer, 3> exp  = {
      Buffer{ 'L', 'i', 'n', 'e', '2', '\n' },
      Buffer{ 'L', 'i', 'n', 'e', '3', '\n' }
    };

    EXPECT_EQ(2, f.read(buf));
    EXPECT_TRUE(f.eof());
    EXPECT_EQ(buf, exp);
  }
  auto pos = std::fpos_t{};
  {
    buf.fill('\0');
    f.seek(-recSize, File::Seek::End);
    EXPECT_EQ(2 * recSize, f.tell());
    pos = f.getpos();
    auto len = f.read(buf);
    EXPECT_EQ(recSize, len);
    static const gsl::czstring exp = "Line3\n";
    EXPECT_STREQ(buf.data(), exp);
    f.setpos(pos);
    EXPECT_EQ(2 * recSize, f.tell());
  }
} // SeekAndTell

TEST_F(FileTest, CharInputOutput) {
  {
    File f{tmpfile_path, std::ios_base::in};
    const char c = 'X';
    EXPECT_THROW({f.putc(c);}, std::system_error);
    EXPECT_TRUE(f.error());
  }
  {
    File f{tmpfile_path, std::ios_base::out};
    for (const auto c : ThreeLineArr)
      f.putc(c);
    f.rewind();
    EXPECT_EQ(f.tell(), 0L);
    EXPECT_THROW({ (void) f.getc(); }, std::system_error);
    EXPECT_TRUE(f.error());
    f.clearerr();
    EXPECT_FALSE(f.error());
  }
  {
    std::optional<char> c;
    File f{tmpfile_path, std::ios_base::in};
    std::string s;
    while ((c = f.getc())) {
      EXPECT_NE(*c, '\0');
      s.push_back(*c);
    }
    EXPECT_TRUE(f.eof());
    EXPECT_FALSE(f.error());
    auto exp = std::string{ThreeLineStr};
    EXPECT_EQ(s, std::string{ThreeLineStr});
    const char x = 'X';
    EXPECT_TRUE(f.ungetc(x));
    EXPECT_FALSE(f.eof());
    EXPECT_EQ(*f.getc(), x);
  }
} // CharInputOutput
