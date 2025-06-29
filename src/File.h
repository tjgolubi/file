/// @file File.h
/// @brief Safe std::FILE* wrapper with exception-based error handling.
///
/// This header defines the `tjg::File` class, a thin RAII-style wrapper
/// around a `std::FILE*` that supports exception-based error reporting,
/// filename retention, and safe destructor behavior. It includes helpers
/// for mode string conversion, error formatting, and line-based reading.
///
/// @author Terry Golubiewski
/// @date 2025
/// @copyright
///   Copyright 2025 Terry Golubiewski. All rights reserved.
///   Distributed under the MIT License.
///
/// @see File.cpp for implementation details.

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

private:
  /// Represents end-of-file.
  static constexpr auto Eof = traits_t::eof();

  std::FILE* _fp = nullptr;  /// Wrapped pointer.
  path _name;                /// File name or user-defined label.

  /// Generates an string for an exception "what" message of the form:
  /// "File <name>: <what>".
  std::string error_string(str_arg what) const;

  /// Throws a std::system_error with a context message.
  [[noreturn]] void throw_error(std::errc ec, str_arg what) const {
    throw std::system_error{std::make_error_code(ec), error_string(what)};
  }

  /// Throws a std::system_error with a context message.
  [[noreturn]] void throw_errno(str_arg what) const {
    auto err = errno;
    if (err != 0) {
      auto ec = std::error_code{err, std::system_category()};
      throw std::system_error{ec, error_string(what)};
    }
    throw_error(std::errc::io_error, what);
  }

  /// Convert a `char_t` to an `int`.
  static inline constexpr int IntChar(char_t c)
    { return static_cast<int>(static_cast<unsigned char>(c)); }

public:
  // Only the next four member functions are valid after calling close().

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

  /// @brief Closes the file using std::fclose
  /// Does not clear the file name.
  /// @see https://en.cppreference.com/w/c/io/fclose
  /// @throws std::system_error
  void close() {
    if (_fp == nullptr)
      return;
    auto save = ResetErrno{};
    auto err = std::fclose(_fp);
    _fp = nullptr;
    if (err != 0) throw_errno("fclose");
  }

private:
  /// Converts an std::ios_base::openmode to a C-style open mode string.
  /// Throws if the `mode` is invalid.
  /// @return A file open mode string.
  /// @throw std::system_error
  static czstring ModeStr(std::ios_base::openmode mode);

  class ResetErrno {
    int _errno;
  public:
    ResetErrno() : _errno{errno} { errno = 0; }
    ~ResetErrno() { errno = _errno; }
  }; // ResetErrno

  /// Opens the file using std::fopen
  /// @see https://en.cppreference.com/w/c/io/fopen
  /// @throws std::system_error
  void open(str_arg mode) {
    auto save = ResetErrno{};
    _fp = std::fopen(_name.c_str(), mode);
    if (_fp == nullptr) throw_errno("fopen");
  } // open

public:
  /// No file, no name.
  File() noexcept = default;

  /// Takes ownership of a std::FILE*
  explicit File(not_null<FILE*> fp_, path name_=path{}) noexcept
    : _fp(fp_), _name(std::move(name_)) { }

  /// Opens a file with a C-style open mode string.
  /// @see https://en.cppreference.com/w/c/io/fopen
  /// @throws std::system_error
  explicit File(std::filesystem::path name, str_arg mode)
    : _fp{}, _name{std::move(name)}
  { open(mode); }

  /// Opens a file with an `openmode` bitmask.
  /// @see https://en.cppreference.com/w/c/io/fopen
  /// @throws std::system_error
  explicit File(std::filesystem::path name, std::ios_base::openmode mode)
    : _fp{}, _name{std::move(name)}
  {
    auto str = ModeStr(mode);
    if (!str) throw_error(std::errc::invalid_argument, "invalid open mode");
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
  /// @throws std::system_error
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

  /// Resets the error flags and the end-of-file indicator.
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
  /// @throws std::system_error
  void flush() noexcept {
    auto save = ResetErrno{};
    if (std::fflush(_fp) != 0) throw_errno("fflush");
  }

  /// Reads a single character using std::fgetc
  /// @see https://en.cppreference.com/w/c/io/fgetc
  /// @throws std::system_error, gsl::narrowing_error
  [[nodiscard]]
  std::optional<char_t> getc() {
    auto save = ResetErrno{};
    auto ch = std::fgetc(_fp);
    if (ch != Eof)
      return std::optional<char_t>{gsl::narrow<char_t>(ch)};
    if (!eof()) throw_errno("fgetc");
    return std::optional<char_t>{};
  }

  /// Standard buffer with default optimal size for this platform.
  template<std::size_t N=BUFSIZ>
  using Buffer = std::array<char_t, N>;

  /// Reads a C-style string using std::fgets
  ///
  /// Reads at most `count-1` characters from the file stream and
  /// stores them in the character array pointed to by `str`. Parsing stops
  /// if a newline character is found, in which case `str` will contain that
  /// newline character, or if end-of-file occurs. If bytes are read and
  /// no errors occur, writes a null character at the position immediately
  /// after the last character written to `str`.
  ///
  /// @param str    Pointer to an element of a char array
  /// @param count  Maximum number of characters to read (typically the length of `str`) 
  /// @see https://en.cppreference.com/w/c/io/fgetc
  /// @requires count > 1.
  /// @return `true` if successful, `false` if end-of-file
  /// @throws std::system_error
  bool gets(not_null<zstring> str, int count) {
    Expects(count > 1);
    auto save = ResetErrno{};
    auto rval = std::fgets(str, count, _fp);
    if (rval != nullptr)
      return true;
    if (!eof()) throw_errno("fgets");
    return false;
  }

  /// Reads a C-style string into a std::array<char_t> using std::fgets
  ///
  /// Reads at most `str.size()-1` characters from the file stream and stores
  /// them in the Buffer str. Parsing stops if a newline character is found,
  /// in which case str will contain that newline character, or if end-of-file
  /// occurs. If bytes are read and no errors occur, writes a null character
  /// at the position immediately after the last character written to str.

  /// If the end-of-file condition is encountered, sets the eof indicator on
  /// the file stream (see eof()). This is only a failure if it causes no bytes
  /// to be read, in which case an exception is thrown and the contents
  /// of the array pointed to by str are not altered (i.e. the first byte is
  /// not overwritten with a null character).
  ///
  /// If the failure has been caused by some other error, sets the error
  /// indicator (see error()) of the stream and throws an exception.
  /// The contents of the array pointed to by str are indeterminate (it may not
  /// even be null-terminated).

  /// @param str  An std::array<char_t>
  /// @see https://en.cppreference.com/w/c/io/fgetc
  /// @requires count > 1.
  /// @throws std::system_error
  template<std::size_t N=BUFSIZ>
  bool gets(Buffer<N>& str) { return gets(str.data(), std::ssize(str)); }

  /// C-style formatted output using std::fprintf
  /// @see https://en.cppreference.com/w/c/io/fprintf
  /// @throws std::system_error
  template<typename... Args>
  int printf(str_arg fmt, Args&&... args) {
    auto save = ResetErrno{};
    int rval = std::fprintf(_fp, fmt, std::forward<Args>(args)...);
    if (rval < 0) throw_errno("fprintf");
    return rval;
  }

  /// Writes a single character using std::fputc
  /// @see https://en.cppreference.com/w/c/io/fputc
  /// @throws std::system_error
  void putc(char_t ch) {
    auto save = ResetErrno{};
    auto rval = std::fputc(IntChar(ch), _fp);
    if (rval != ch) throw_errno("fputc");
  }

  /// Writes a C-style string using std::fputs
  /// @see https://en.cppreference.com/w/c/io/fputs
  /// @throws std::system_error
  void puts(not_null<czstring> str) {
    auto save = ResetErrno{};
    auto rval = std::fputs(str, _fp);
    if (rval < 0) throw_errno("fputs");
  }

  /// @brief Reads a block of data using std::fread
  /// Returns the number of records successfully read, which might be less than
  /// `count` if at the end of the input file or an input error occurred.
  /// Throws if nothing was read, but !eof().
  /// @see https://en.cppreference.com/w/c/io/fread
  /// @return The number of records successfully read
  /// @throws std::system_error
  [[nodiscard]]
  std::size_t read(not_null<void*> buf, std::size_t size, std::size_t count) {
    if (size == 0 || count == 0)
      return 0;
    auto save = ResetErrno{};
    std::size_t rval = std::fread(buf, size, count, _fp);
    if (rval == 0 && !eof()) throw_errno("fread");
    return rval;
  }

  /// @brief Reads a std::array using std::fread
  /// Returns the number of items successfully read, which might be less than
  /// buf.size() if at the end of the input file or an input error occurred.
  /// Throws if nothing was read, but !eof()
  /// @see https://en.cppreference.com/w/c/io/fread
  /// @return The number of items successfully read
  /// @throws std::system_error
  template<TriviallyCopyable T, std::size_t N>
  [[nodiscard]] std::size_t read(std::array<T, N>& buf)
  { return read(buf.data(), sizeof(T), buf.size()); }

  /// C-style formatted input using std::fscanf
  /// @see https://en.cppreference.com/w/c/io/fscanf
  /// @return The number of items succesfully parsed and stored.
  /// @throw std::system_error
  [[nodiscard]]
  int scanf(str_arg format, auto&... args) {
    auto save = ResetErrno{};
    auto rval = std::fscanf(_fp, format, args...);
    if (rval < 0 && !(rval == Eof && eof())) throw_errno("fscanf");
    return rval;
  }

  /// Origin for seek().
  enum class Seek {
    Set     = SEEK_SET, /// Relative the beginning of the file
    Current = SEEK_CUR, /// Relative to the current file position
    End     = SEEK_END  /// Relative to the end of the file
  };

  /// Seeks to a given position using std::fseek
  /// @see https://en.cppreference.com/w/c/io/fseek
  /// @throw std::system_error
  void seek(long offset, Seek origin=Seek::Set) {
    auto save = ResetErrno{};
    auto rval = std::fseek(_fp, offset, int(origin));
    if (rval != 0) throw_errno("fseek");
  }

  /// Returns the file position using std::ftell
  /// @see https://en.cppreference.com/w/c/io/ftell
  /// @return Current file position.
  /// @throw std::system_error
  [[nodiscard]]
  long tell() const {
    auto rval = std::ftell(_fp);
    if (rval == -1L) throw_errno("ftell");
    return rval;
  }

  /// Returns the file position using std::fgetpos
  /// @see https://en.cppreference.com/w/c/io/fgetpos
  /// @return Current file position.
  /// @throw std::system_error
  [[nodiscard]]
  std::fpos_t getpos() const {
    auto result = std::fpos_t{};
    auto rval = std::fgetpos(_fp, &result);
    if (rval != 0) throw_errno("fgetpos");
    return result;
  }

  /// Sets the file position using std::fsetpos
  /// @see https://en.cppreference.com/w/c/io/getpos
  /// @throw std::system_error
  void setpos(const std::fpos_t& pos) {
    auto rval = std::fsetpos(_fp, &pos);
    if (rval != 0) throw_errno("fsetpos");
  }

  /// Writes a block of data using std::fwrite
  /// @see https://en.cppreference.com/w/c/io/fwrite
  /// Throws if the number of records written is not `count`.
  /// @throw std::system_error
  void write(not_null<const void*> buf, std::size_t size, std::size_t count) {
    if (size == 0 || count == 0)
      return;
    auto save = ResetErrno{};
    auto rval = std::fwrite(buf, size, count, _fp);
    if (rval != count) throw_errno("fwrite");
  }

  /// Writes a std::array using std::fwrite
  /// @see https://en.cppreference.com/w/c/io/fwrite
  /// Throws if the number of records written is less than `buf.size()`.
  /// @throw std::system_error
  template<TriviallyCopyable T, std::size_t N>
  void write(const std::array<T, N>& buf)
  { return write(buf.data(), sizeof(T), buf.size()); }

  /// Writes a std::basic_string using std::fwrite
  /// @see https://en.cppreference.com/w/c/io/fwrite
  /// Throws if the number of characters written is less than `buf.size()`.
  /// @throw std::system_error
  template<typename Ch, typename Tr=std::char_traits<Ch>>
  void write(const std::basic_string<Ch, Tr>& buf)
  { return write(buf.data(), sizeof(Ch), buf.size()); }

  /// Rewinds the file to the beginning using std::rewind
  /// @see https://en.cppreference.com/w/c/io/rewind
  void rewind() noexcept { std::rewind(_fp); }

  /// Buffering mode for the file stream.
  enum class BufferMode {
    Full = _IOFBF,  /// Read/write the buffer on underflow/overflow
    Line = _IOLBF,  /// Read/write lines (until newline)
    None = _IONBF   /// Unbuffered
  };

  /// Disables buffering using std::setbuf
  /// @see https://en.cppreference.com/w/c/io/setbuf
  void setbuf() noexcept { std::setbuf(_fp, nullptr); }

private:
  /// Sets buffering mode using std::setvbuf
  /// @see https://en.cppreference.com/w/c/io/setvbuf
  /// @throw std::system_error
  void setvbuf(char_t* buf, std::size_t size, BufferMode mode) {
    auto save = ResetErrno{};
    auto rval = std::setvbuf(_fp, buf, static_cast<int>(mode), size);
    if (rval != 0) throw_errno("setvbuf");
  }

public:
  /// Sets buffering mode using std::setvbuf
  /// @see https://en.cppreference.com/w/c/io/setvbuf
  /// @throw std::system_error
  void setbuf(BufferMode mode) { setvbuf(nullptr, 0, mode); }

  /// Sets buffer size and mode using std::setvbuf
  /// @see https://en.cppreference.com/w/c/io/setvbuf
  /// @throw std::system_error
  void setbuf(std::size_t size, BufferMode mode = BufferMode::Full)
  { setvbuf(nullptr, size, mode); }

  /// Sets buffer using std::setvbuf
  /// @see https://en.cppreference.com/w/c/io/setvbuf
  /// @throw std::system_error
  void setbuf(not_null<char_t*> buf, std::size_t size,
              BufferMode mode = BufferMode::Full)
  { setvbuf(buf, size, mode); }

  /// Sets buffer using std::array and std::setvbuf
  /// @see https://en.cppreference.com/w/c/io/setvbuf
  /// @throw std::system_error
  template<std::size_t N>
  void setbuf(std::array<char_t, N>& buf, BufferMode mode = BufferMode::Full)
  { setvbuf(buf.data(), N, mode); }

  /// Pushes a character back using std::ungetc
  /// @see https://en.cppreference.com/w/c/io/ungetc
  /// @return `true` if successful; otherwise `false`
  [[nodiscard]]
  bool ungetc(char_t ch) noexcept {
    auto rval = std::ungetc(IntChar(ch), _fp);
    return (rval != Eof);
  }

}; // File

/// Swap File
inline void swap(File& lhs, File& rhs) noexcept { lhs.swap(rhs); }

} // tjg
