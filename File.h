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

template<typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template<typename... Args>
[[noreturn]]
void LogTerminate(std::format_string<Args...> fmt, Args&&... args) noexcept {
  std::println(std::clog, fmt, std::forward<Args>(args)...);
  std::clog.flush();
  std::terminate();
} // LogTerminate

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

  std::string error_string(const std::string& what) const
  { return "File " + _name.generic_string() + ": " + what; }

  [[noreturn]] void throw_error(const std::string& what) const
    { throw Error{error_string(what)}; }

  [[noreturn]] void throw_errno(const std::string& what) const {
    auto err = std::error_code{errno, std::system_category()};
    throw std::system_error{err, error_string(what)};
  }

#ifdef _POSIX_VERSION
#define TJG_FILE_POSIX_THROW(what) throw_errno(what)
#else
#define TJG_FILE_POSIX_THROW(what) throw_error(what)
#endif

  static inline constexpr int IntChar(char_t c)
    { return static_cast<int>(static_cast<unsigned char>(c)); }

public:
  // Only these 4 member functions are valid after calling close().
  operator std::FILE*() noexcept { return _fp; }
  bool is_open() const noexcept { return (_fp != nullptr); }
  const path& name() const noexcept { return _name; }

  ~File() noexcept {
    if (_fp == nullptr)
      return;
    auto rval = std::fclose(_fp);
    if (rval == 0)
      return;
    LogTerminate("~File: fclose returned {}: errno={}",
                                                   rval, std::strerror(errno));
  }

  void close() {
    if (_fp == nullptr)
      return;
    auto err = std::fclose(_fp);
    _fp = nullptr;
    if (err == 0) throw_error("fclose");
  }

  void open(path name, str_arg mode) {
    close();
    _name = std::move(name);
    _fp   = std::fopen(_name.c_str(), mode);
    if (_fp == nullptr) TJG_FILE_POSIX_THROW("fopen");
  } // open

  File() noexcept = default;

  explicit File(not_null<FILE*> fp_, path name_=path{}) noexcept
    : _fp(fp_), _name(name_) { }

  explicit File(std::filesystem::path name, str_arg mode)
  { open(std::move(name), mode); }

  File(const File&) = delete;
  File& operator=(const File& f) = delete;

  File(File&& f) noexcept : _fp{f._fp}, _name{std::move(f._name)}
  { f._fp = nullptr; }

  File& operator=(File&& f) {
    if (&f == this)
      return *this;
    close();
    _fp   = f._fp;
    _name = std::move(f._name);
    f._fp = nullptr;
    return *this;
  }

  void swap(File& f) noexcept {
    std::swap(_fp,   f._fp);
    std::swap(_name, f._name);
  }

  void clearerr()    noexcept { std::clearerr(_fp); }
  bool eof()   const noexcept { return (std::feof(_fp)   != 0); }
  bool error() const noexcept { return (std::ferror(_fp) != 0); }
  bool flush()       noexcept { return (std::fflush(_fp) == 0); }

  std::optional<char_t> getc() {
    auto ch = std::fgetc(_fp);
    if (ch != Eof)
      return std::optional<char_t>{gsl::narrow<char_t>(ch)};
    if (!eof()) throw_error("fgetc");
    return std::optional<char_t>{};
  }

  void gets(not_null<zstring> s, int count) {
    Expects(count > 1);
    auto rval = std::fgets(s, count, _fp);
    if (rval == nullptr) TJG_FILE_POSIX_THROW("fgets");
  }

  template<typename... Args>
  int printf(str_arg fmt, Args&&... args) {
    int rval = std::fprintf(_fp, fmt, std::forward<Args>(args)...);
    if (rval < 0) TJG_FILE_POSIX_THROW("fprintf");
    return rval;
  }

  void putc(char_t ch) {
    auto rval = std::fputc(IntChar(ch), _fp);
    if (rval == Eof) throw_error("fputc");
  }

  void puts(not_null<zstring> s) {
    auto rval = std::fputs(s, _fp);
    if (rval < 0) throw_error("fputs");
  }

  std::size_t read(not_null<void*> buf, std::size_t size, std::size_t count) {
    if (size == 0 || count == 0)
      return 0;
    std::size_t rval = std::fread(buf, size, count, _fp);
    if (rval == 0 && !eof() && error()) throw_error("fread");
    return rval;
  }

  template<TriviallyCopyable T>
  std::size_t read(std::span<T> buf)
  { return read(buf.data(), sizeof(T), buf.size()); }

  int scanf(str_arg format, auto&... args) {
    auto rval = std::fscanf(_fp, format, args...);
    if (rval == Eof && !eof() && error()) throw_error("fscanf");
    return rval;
  }

  enum class Seek { Set=SEEK_SET, Current=SEEK_CUR, End=SEEK_END };

  void seek(long offset, Seek origin=Seek::Set) {
    auto rval = std::fseek(_fp, offset, int(origin));
    if (rval != 0) TJG_FILE_POSIX_THROW("fseek");
  }

  long tell() const {
    auto rval = std::ftell(_fp);
    if (rval == -1L) throw_errno("ftell");
    return rval;
  }

  std::fpos_t getpos() const {
    auto result = std::fpos_t{};
    auto rval = std::fgetpos(_fp, &result);
    if (rval != 0) throw_errno("fgetpos");
    return result;
  }

  void setpos(const std::fpos_t& pos) {
    auto rval = std::fsetpos(_fp, &pos);
    if (rval != 0) throw_errno("fsetpos");
  }

  std::size_t write(not_null<const void*> buf, std::size_t size,
                    std::size_t count)
  {
    if (size == 0 || count == 0)
      return 0;
    auto rval = std::fwrite(buf, size, count, _fp);
    if (rval == 0) throw_error("fwrite");
    return rval;
  }

  template<TriviallyCopyable T>
  std::size_t write(std::span<T> buf)
  { return write(buf.data(), sizeof(T), buf.size()); }

  void rewind() noexcept { std::rewind(_fp); }

  enum class BufferMode { Full = _IOFBF, Line = _IOLBF, None = _IONBF };

  using Buffer = std::array<char_t, BUFSIZ>;

  void setbuf() noexcept { std::setbuf(_fp, nullptr); }

private:
  void setvbuf(char_t* buf, std::size_t size, BufferMode mode) {
    auto rval = std::setvbuf(_fp, buf, static_cast<int>(mode), size);
    if (rval != 0) throw_error("setvbuf");
  }

public:
  void setbuf(BufferMode mode) { setvbuf(nullptr, 0, mode); }

  void setbuf(std::size_t size, BufferMode mode = BufferMode::Full)
  { setvbuf(nullptr, size, mode); }

  void setbuf(not_null<char_t*> buf, std::size_t size,
              BufferMode mode = BufferMode::Full)
  { setvbuf(buf, size, mode); }

  template<std::size_t N>
  void setbuf(std::array<char_t, N>& buf, BufferMode mode = BufferMode::Full)
  { setvbuf(buf.data(), N, mode); }

  bool ungetc(char_t ch) noexcept {
    auto rval = std::ungetc(IntChar(ch), _fp);
    return (rval != Eof);
  }

}; // File

inline void swap(File& lhs, File& rhs) noexcept { lhs.swap(rhs); }

} // tjg

#endif
