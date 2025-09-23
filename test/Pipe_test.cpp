/// @file
/// GoogleTest for Pipe functionality in File.hpp (POSIX/Git Bash).
///
/// Covers:
///  - Reading from a pipe that prints a known file (`cat`).
///  - Writing to a child via stdin (child discards to /dev/null).
///  - EOF behavior for a command with no output (`true`).
///  - Non-seekable stream behavior (seek throws).
///  - Close status check using POSIX wait macros when available.
///
/// Assumes a POSIX-y environment (Git Bash / MSYS2 / MinGW64) with
/// /bin/sh and coreutils. If <sys/wait.h> is unavailable, the tests
/// still pass by asserting close() != -1 only.
///
/// @author Terry Golubiewski
/// @copyright 2025 Terry Golubiewski, all rights reserved.

#include "File.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>
#include <array>
#include <cstdio>

namespace fs = std::filesystem;
using tjg::File;
using tjg::Pipe;

namespace {

static std::string CatCmd(const fs::path& p) {
  std::string cmd = "cat \"";
  cmd += p.string();
  cmd += "\"";
  return cmd;
}

} // local

TEST(Pipe, ReadBackKnownFileViaPipe) {
  static const auto kText = std::to_array(
    "alpha\n"
    "beta\n"
    "gamma\n");

  auto tmp = fs::temp_directory_path() / "tjg_pipe_read_test.txt";
  {
    File f{tmp, "wb"};
    ASSERT_TRUE(f.is_open());
    f.write(kText);
  }

  Pipe p{CatCmd(tmp), "r"};
  ASSERT_TRUE(p.is_open());

  std::string got;
  got.reserve(64);
  std::optional<char> c;
  while ((c = p.getc()))
    got.push_back(*c);
  EXPECT_TRUE(p.eof());
  EXPECT_FALSE(p.error());
  const auto expect = std::string_view{kText.begin(), kText.end()};
  EXPECT_EQ(got, expect);

  int status = p.close();
  EXPECT_NE(status, -1);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(WEXITSTATUS(status), 0);
}

TEST(Pipe, WriteToChildStdIn) {
  Pipe p{"sh -c 'cat >/dev/null'", "w"};
  ASSERT_TRUE(p.is_open());

  const std::string payload(4096, 'X');
  p.write(payload);

  int status = p.close();
  EXPECT_NE(status, -1);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(WEXITSTATUS(status), 0);
}

TEST(Pipe, EOFWithEmptyOutput) {
  Pipe p{"sh -c 'true' </dev/null", "r"};
  ASSERT_TRUE(p.is_open());

  auto c = p.getc();
  EXPECT_FALSE(c.has_value());
  EXPECT_TRUE(p.eof());
  EXPECT_FALSE(p.error());

  int status = p.close();
  EXPECT_NE(status, -1);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(WEXITSTATUS(status), 0);
}

TEST(Pipe, SeekOnPipeThrows) {
  Pipe p{"sh -c 'cat >/dev/null'", "w"};
  ASSERT_TRUE(p.is_open());

  EXPECT_THROW({ p.seek(0, Pipe::Seek::Set); }, std::system_error);

  int status = p.close();
  EXPECT_NE(status, -1);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(WEXITSTATUS(status), 0);
}

TEST(Pipe, NonZeroExitStatus) {
  // Child exits with 7. On POSIX, pclose returns a wait status.
  Pipe p{"sh -c 'exit 7'", "r"};
  ASSERT_TRUE(p.is_open());

  // No output, immediate EOF.
  auto c = p.getc();
  EXPECT_FALSE(c.has_value());

  int status = p.close();
  EXPECT_NE(status, -1);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(WEXITSTATUS(status), 7);
}
