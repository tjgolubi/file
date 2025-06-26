#ifndef TJG_FILE_H
#define TJG_FILE_H
#pragma once

#include <gsl/gsl>

#include <optional>
#include <string>
#include <array>
#include <filesystem>
#include <iostream>
#include <print>
#include <system_error>
#include <exception>
#include <utility>
#include <type_traits>
#include <cstdio>

namespace tjg {

/// A contiguous sequence of bytes.
/// @see https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable
template<typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

/// Print a formatted message to std::clog and terminate.
/// Typically used to report errors from within a destructor.
template<typename... Args>
[[noreturn]]
void LogTerminate(std::format_string<Args...> fmt, Args&&... args) noexcept {
  std::println(std::clog, fmt, std::forward<Args>(args)...);
  std::clog.flush();
  std::terminate();
} // LogTerminate

/// @brief Safe std::FILE* wrapper with exception-based error handling.
class File {
public:
  using path = std::filesystem::path;
  template<typename T> using not_null = gsl::not_null<T>;
  using zstring  = gsl::zstring;
  using czstring = gsl::czstring;

  using str_rv   = not_null<zstring>;
  using str_arg  = not_null<czstring>;

  using char_t   = char;
  using traits_t = std::char_traits<char_t>;

  /// Exception thrown when error is not set.
  class Error: public std::exception {
    const std::string _what;
  public:
    czstring what() const noexcept override { return _what.c_str(); }
    explicit Error(const std::string& what_) : _what{what_} { }
  }; // Error

private:
  static constexpr auto Eof = traits_t::eof();

  std::FILE* _fp = nullptr;
  path _name;

  std::string error_string(str_arg what) const;

  [[noreturn]] void throw_error(str_arg what) const
    { throw Error{error_string(what)}; }

  [[noreturn]] void throw_errno(str_arg what) const {
    auto err = std::error_code{errno, std::system_category()};
    throw std::system_error{err, error_string(what)};
  }

  [[noreturn]] void throw_posix(str_arg what) const {
#ifdef _POSIX_VERSION
    throw_errno(what);
#else
    throw_error(what);
#endif
  }

  static inline constexpr int IntChar(char_t c)
    { return static_cast<int>(static_cast<unsigned char>(c)); }

public:
  // Only these 4 member functions are valid after calling close().

  /// Implicitly returns the owned FILE*.
  operator std::FILE*() noexcept { return _fp; }

  /// Returns `true` if the a file is owned.
  bool is_open() const noexcept { return (_fp != nullptr); }

  /// Returns the name of the owned file
  /// The filename is retained even after the file is closed.
  /// The filename may be empty even for an open file.
  const path& name() const noexcept { return _name; }

  /// Safely closes owned file, terminating on error.
  /// Uses LogTerminate() with an appropriate error message.
  ~File() noexcept {
    if (_fp == nullptr)
      return;
    auto rval = std::fclose(_fp);
    if (rval == 0)
      return;
    LogTerminate("~File: fclose returned {}: errno={}",
                                                   rval, std::strerror(errno));
  }

  /// Closes the file using std::fclose
  /// @see https://en.cppreference.com/w/c/io/fclose
  void close() {
    if (_fp == nullptr)
      return;
    auto err = std::fclose(_fp);
    _fp = nullptr;
    if (err != 0) throw_error("fclose");
  }

private:
  static czstring ModeStr(std::ios_base::openmode mode);

  /// Opens the file using std::fopen
  /// @see https://en.cppreference.com/w/c/io/fopen
  void open(str_arg mode) {
    _fp = std::fopen(_name.c_str(), mode);
    if (_fp == nullptr) throw_posix("fopen");
  } // open

public:
  /// No file, no name.
  File() noexcept = default;

  /// Takes ownership of a std::FILE*
  explicit File(not_null<FILE*> fp_, path name_=path{}) noexcept
    : _fp(fp_), _name(std::move(name_)) { }

  /// Opens a file
  explicit File(std::filesystem::path name, str_arg mode)
    : _fp{}, _name{std::move(name)}
  { open(mode); }

  /// Opens a file
  explicit File(std::filesystem::path name, std::ios_base::openmode mode)
    : _fp{}, _name{std::move(name)}
  {
    auto str = ModeStr(mode);
    if (!str) throw_error("invalid open mode");
    open(str);
    if (mode & std::ios_base::ate)
      seek(0, Seek::End);
  }

  /// Move-only
  File(const File&) = delete;
  File& operator=(const File& f) = delete;

  /// Transfer ownership
  File(File&& f) noexcept : _fp{f._fp}, _name{std::move(f._name)}
  { f._fp = nullptr; }

  /// Transfer ownership
  File& operator=(File&& f) {
    if (&f == this)
      return *this;
    close();
    _fp   = f._fp;
    _name = std::move(f._name);
    f._fp = nullptr;
    return *this;
  }

  /// Swap ownership.
  void swap(File& f) noexcept {
    std::swap(_fp,   f._fp);
    std::swap(_name, f._name);
  }

  /// Clears a file stream's error state using std::clearerr
  /// @see https://en.cppreference.com/w/c/io/clearerr
  void clearerr()    noexcept { std::clearerr(_fp); }

  /// Checks for end-of-file using std::feof
  /// @see https://en.cppreference.com/w/c/io/feof
  [[nodiscard]]
  bool eof()   const noexcept { return (std::feof(_fp)   != 0); }

  /// Checks the file stream error state using std::ferror
  /// @see https://en.cppreference.com/w/c/io/ferror
  [[nodiscard]]
  bool error() const noexcept { return (std::ferror(_fp) != 0); }

  /// Flushes the file stream buffer using std::fflush
  /// @see https://en.cppreference.com/w/c/io/fflush
  [[nodiscard]]
  bool flush()       noexcept { return (std::fflush(_fp) == 0); }

  /// Reads a single character using std::fgetc
  /// @see https://en.cppreference.com/w/c/io/fgetc
  [[nodiscard]]
  std::optional<char_t> getc() {
    auto ch = std::fgetc(_fp);
    if (ch != Eof)
      return std::optional<char_t>{gsl::narrow<char_t>(ch)};
    if (!eof()) throw_error("fgetc");
    return std::optional<char_t>{};
  }

  /// Reads a C-style string using std::fgets
  /// @see https://en.cppreference.com/w/c/io/fgetc
  void gets(not_null<zstring> s, int count) {
    Expects(count > 1);
    auto rval = std::fgets(s, count, _fp);
    if (rval == nullptr) throw_posix("fgets");
  }

  /// C-style formatted output using std::fprintf
  /// @see https://en.cppreference.com/w/c/io/fprintf
  template<typename... Args>
  int printf(str_arg fmt, Args&&... args) {
    int rval = std::fprintf(_fp, fmt, std::forward<Args>(args)...);
    if (rval < 0) throw_posix("fprintf");
    return rval;
  }

  /// Writes a single character using std::fputc
  /// @see https://en.cppreference.com/w/c/io/fputc
  void putc(char_t ch) {
    auto rval = std::fputc(IntChar(ch), _fp);
    if (rval == Eof) throw_error("fputc");
  }

  /// Writes a C-style string using std::fputs
  /// @see https://en.cppreference.com/w/c/io/fputs
  void puts(not_null<zstring> s) {
    auto rval = std::fputs(s, _fp);
    if (rval < 0) throw_error("fputs");
  }

  /// Reads a block of data using std::fread
  /// @see https://en.cppreference.com/w/c/io/fread
  [[nodiscard]]
  std::size_t read(not_null<void*> buf, std::size_t size, std::size_t count) {
    if (size == 0 || count == 0)
      return 0;
    std::size_t rval = std::fread(buf, size, count, _fp);
    if (rval == 0 && !eof() && error()) throw_error("fread");
    return rval;
  }

  /// Reads a std::span using std::fread
  /// @see https://en.cppreference.com/w/c/io/fread
  template<TriviallyCopyable T>
  [[nodiscard]] std::size_t read(std::span<T> buf)
  { return read(buf.data(), sizeof(T), buf.size()); }

  /// C-style formatted input using std::fscanf
  /// @see https://en.cppreference.com/w/c/io/fscanf
  [[nodiscard]]
  int scanf(str_arg format, auto&... args) {
    auto rval = std::fscanf(_fp, format, args...);
    if (rval == Eof && !eof() && error()) throw_error("fscanf");
    return rval;
  }

  enum class Seek { Set=SEEK_SET, Current=SEEK_CUR, End=SEEK_END };

  /// Seeks to a given position using std::fseek
  /// @see https://en.cppreference.com/w/c/io/fseek
  void seek(long offset, Seek origin=Seek::Set) {
    auto rval = std::fseek(_fp, offset, int(origin));
    if (rval != 0) throw_posix("fseek");
  }

  /// Returns the file position using std::ftell
  /// @see https://en.cppreference.com/w/c/io/ftell
  [[nodiscard]]
  long tell() const {
    auto rval = std::ftell(_fp);
    if (rval == -1L) throw_errno("ftell");
    return rval;
  }

  /// Returns the file position using std::fgetpos
  /// @see https://en.cppreference.com/w/c/io/fgetpos
  [[nodiscard]]
  std::fpos_t getpos() const {
    auto result = std::fpos_t{};
    auto rval = std::fgetpos(_fp, &result);
    if (rval != 0) throw_errno("fgetpos");
    return result;
  }

  /// Sets the file position using std::fsetpos
  /// @see https://en.cppreference.com/w/c/io/getpos
  void setpos(const std::fpos_t& pos) {
    auto rval = std::fsetpos(_fp, &pos);
    if (rval != 0) throw_errno("fsetpos");
  }

  /// Writes a block of data using std::fwrite
  /// @see https://en.cppreference.com/w/c/io/fwrite
  void write(not_null<const void*> buf, std::size_t size, std::size_t count) {
    if (size == 0 || count == 0)
      return;
    auto rval = std::fwrite(buf, size, count, _fp);
    if (rval != count) throw_error("fwrite");
  }

  /// Writes a std::span using std::fwrite
  /// @see https://en.cppreference.com/w/c/io/fwrite
  template<TriviallyCopyable T>
  void write(std::span<T> buf)
  { return write(buf.data(), sizeof(T), buf.size()); }

  /// Rewinds the file to the beginning using std::rewind
  /// @see https://en.cppreference.com/w/c/io/rewind
  void rewind() noexcept { std::rewind(_fp); }

  enum class BufferMode { Full = _IOFBF, Line = _IOLBF, None = _IONBF };

  using Buffer = std::array<char_t, BUFSIZ>;

  /// Disables buffering using std::setbuf
  /// @see https://en.cppreference.com/w/c/io/setbuf
  void setbuf() noexcept { std::setbuf(_fp, nullptr); }

private:
  void setvbuf(char_t* buf, std::size_t size, BufferMode mode) {
    auto rval = std::setvbuf(_fp, buf, static_cast<int>(mode), size);
    if (rval != 0) throw_error("setvbuf");
  }

public:
  /// Sets buffering mode using std::setvbuf
  /// @see https://en.cppreference.com/w/c/io/setvbuf
  void setbuf(BufferMode mode) { setvbuf(nullptr, 0, mode); }

  /// Sets buffer size and mode using std::setvbuf
  /// @see https://en.cppreference.com/w/c/io/setvbuf
  void setbuf(std::size_t size, BufferMode mode = BufferMode::Full)
  { setvbuf(nullptr, size, mode); }

  /// Sets buffer using std::setvbuf
  /// @see https://en.cppreference.com/w/c/io/setvbuf
  void setbuf(not_null<char_t*> buf, std::size_t size,
              BufferMode mode = BufferMode::Full)
  { setvbuf(buf, size, mode); }

  /// Sets buffer using std::array and std::setvbuf
  /// @see https://en.cppreference.com/w/c/io/setvbuf
  template<std::size_t N>
  void setbuf(std::array<char_t, N>& buf, BufferMode mode = BufferMode::Full)
  { setvbuf(buf.data(), N, mode); }

  /// Pushes a character back using std::ungetc
  /// @see https://en.cppreference.com/w/c/io/ungetc
  [[nodiscard]]
  bool ungetc(char_t ch) noexcept {
    auto rval = std::ungetc(IntChar(ch), _fp);
    return (rval != Eof);
  }

}; // File

inline void swap(File& lhs, File& rhs) noexcept { lhs.swap(rhs); }

} // tjg

#endif
