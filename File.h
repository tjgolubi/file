namespace tjg {

template<typename... Args>
[[noreturn]]
void LogTerminate(std::format_string<Args...> fmt, Args&&... args) noexcept {
  std::println(std::clog, fmt, args...);
  std::clog.flush();
  std::terminate();
} // LogTerminate

class File {
public:
  using path = std::filesystem::path;

  using czstring = gsl::czstring;
  using not_null = gsl::not_null;
  using str_arg  = not_null<czstring>;

  inline constexpr auto EOF = std::char_traits<char>::eof;

private:
  [[noreturn]]
  static void DtorError(str_arg what) noexcept {
    auto err = std:error_code{errno, std::system_category{}};
    LogTerminate("File::DtorError: {}: {}", what, err.message());
  }

  static void Close(std::FILE* fp) noexcept {
    if (std::fclose(fp) != 0)
      DtorError("fclose");
  }

  using unique_file_t = std::unique_ptr<std::FILE, decltype(&Close)>;
  unique_file_t _fp;
  path _name;

public:
  // Only these 2 member functions are valid after calling close().
  bool is_open() const noexcept { return bool(_fp); }
  const path& path() const { return _name; }

  not_null<FILE*> get() { return _fp.get(); }

private:
  inline void check(bool cond, const std::string& what) {
    if (cond)
      return;
    auto err = std::error_code{errno, std::system_category{}};
    throw std::system_error{err, _name + ": " + what};
  } // check

  void do_flush() {
    auto err = std::fflush(get());
    check((err == 0), "fflush");
  }

  void do_close() {
    auto err = std::fclose(get());
    _fp.reset();
    return check((err == 0), "fclose");
  }

  void do_open(path name, str_arg mode) {
    _name = std::move(name);
    auto fp = std::fopen(_name.c_str(), mode);
    check((fp != nullptr), "fopen");
    _fp.reset(fp);
  } // do_open

public:
  void open(path name, str_arg mode)
  { do_open(std::move(name), mode); }

  File() = default;

  explicit File(std::filesystem::path name, str_arg mode)
  { do_open(name, mode); }

  virtual ~File() = default;

  File(const File&) = delete;
  File(File&& f) = default;
  File& operator=(const File&) = default;

  void swap(File& other) noexcept { std::swap(_fp, other._fp); }

  bool close() { return do_close(); }

  void clearerr() noexcept { std::clearerr(get()); }

  bool eof()   const noexcept { return (std::feof(get())   != 0); }
  bool error() const noexcept { return (std::ferror(get()) != 0); }
  void flush() { check((std::fflush(get()) == 0), "fflush"); }

  int getc() {
    int ch = std::fgetc(get());
    if (ch == EOF)
      check(!error(), "fgetc");
    return ch;
  }

  std::fpos_t getpos() const {
    auto rval = std::fpos_t{};
    check((std::fgetpos(get(), &rval) == 0), "fgetpos");
    return rval;
  }

  std::string_view gets(std::string_view sv) {
    auto rval = std::fgets(sv.begin(), sv.size(), get());
    check((rval != nullptr), "fgets");
    return sv;
  }

  int printf(str_arg fmt, auto... args) {
    int rval = std::fprintf(get(), format, args...);
    check((rval >= 0), "fprintf");
    return rval;
  }

  void putc(int ch)
  { check((std::fputc(ch, get()) == EOF), "fputc"); }

  void puts(std::string_view sv) {
    auto rval = std::fputs(sv.begin(), sv.size(), get());
    check((rval != EOF), "fputs");
  }

private:
  std::size_t read(void* buf, std::size_t size, std::size_t count) {
    if (size == 0 || count == 0)
      return 0;
    auto rval = std::fread(buf, size, count, get());
    if (rval == 0)
      check(!error(), "fread");
    return rval;
  }

public:
  template<typename T>
  concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

  template<TriviallyCopyable T>
  std::size_t read(std::span<T> buf)
  { return read(buf.data(), sizeof(T), buf.size()); }

  int scanf(str_arg format, auto&... args) {
    auto rval = std::fscanf(get(), format, args...);
    if (rval == EOF)
      check(!error(), "fscanf");
    return rval;
  }

  enum class Seek { Set=SEEK_SET, Current=SEEK_CUR, End=SEEK_END };

  void seek(long offset, SeekEnum origin=Seek::Set) {
    auto rval =std::fseek(get(), offset, int(origin));
    check((rval == 0), "fseek");
  }

  void setpos(const std::fpos_t& pos)
  { check(std::fsetpos(get(), &pos) == 0, "fsetpos"); }

  long tell() const {
    auto rval = std::ftell(get());
    check((rval != -1L), "ftell");
    return rval;
  }

private:
  std::size_t write(const void* buf, std::size_t size, std::size_t count) {
    if (size == 0 || count == 0)
      return 0;
    auto rval = std::fwrite(buf, size, count, get());
    if (rval == 0)
      check(!error(), "fwrite");
    return rval;
  }

public:
  template<TriviallyCopyable T>
  std::size_t write(std::span<T> buf)
  { return write(buf.data(), sizeof(T), buf.size()); }

  void rewind() { std::rewind(get()); }

  enum class BufferMode { Full = _IOFBF, Line = _IOLBF, None = _IONBF };

  using Buffer = std::array<char, BUFSIZE>;

  void setbuf() { std::setbuf(get(), nullptr); }

  template<std::size_t N>
  void setbuf(std::array<char, N>& buf, BufferMode mode = BufferMode::Full) {
    auto rval = std::setvbuf(get(), buf.data(), int(mode), N);
    check((rval == 0), "setvbuf");
  }

  void setbuf(BufferMode mode, std::size_t size) {
    auto rval = std::setvbuf(get(), nullptr, mode, size);
    check((rval == 0), "setvbuf");
  }

  bool ungetc(int ch) {
    if (ch == EOF)
      return false;
    auto rval = std::ungetc(ch, get());
    return (rval != EOF);
  }

}; // File

#endif
