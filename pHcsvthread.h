#pragma once

#include "pHcsv.h"
#include "pHpool.h"

namespace pH {

namespace csv {

namespace detail {

class processMapped {
 public:
    processMapped(const std::vector<std::string>& header, std::vector<std::string>&& row, const std::function<void(const mapped_row&)>& parse_func)
      : header_(header), row_(std::move(row)), parse_func_(parse_func) {}

    processMapped(processMapped&& other) noexcept : header_(other.header_), row_(std::move(other.row_)), parse_func_(other.parse_func_) {}

    void operator()() const { parse_func_(mapped_row(header_, row_)); }

 private:
  const std::vector<std::string>& header_;
  std::vector<std::string> row_;
  const std::function<void(const mapped_row&)>& parse_func_;
};

class processFlat {
 public:
    processFlat(std::vector<std::string>&& row, const std::function<void(const std::vector<std::string>&)>& parse_func)
      : row_(std::move(row)), parse_func_(parse_func) {}

    processFlat(processFlat&& other) noexcept : row_(std::move(other.row_)), parse_func_(std::move(other.parse_func_)) {}

    void operator()() const { parse_func_(row_); }

 private:
  std::vector<std::string> row_;
  const std::function<void(const std::vector<std::string>&)>& parse_func_;
};

}

void streamRowsThreaded(std::istream& in, size_t num_threads, std::function<void(const mapped_row&)> parse_func) {
  if (num_threads == 0) {
    streamRows(in, parse_func);
    return;
  }
  if (in.bad() || in.fail()) {
    throw std::runtime_error("Bad input");
  }
  std::istreambuf_iterator<char> it(in);
  std::vector<std::string> header = detail::readCsvRow(it);
  pH::pool<detail::processMapped> thread_pool(num_threads);
  while (it != detail::EOCSVF) {
    thread_pool.emplace(header, detail::readCsvRow(it, header.size()), parse_func);
  }
}

inline void streamRowsThreaded(const std::string& filename, size_t num_threads, std::function<void(const mapped_row&)> parse_func) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  streamRowsThreaded(in, num_threads, parse_func);
}

void streamRowsThreaded(std::istream& in, size_t num_threads, std::function<void(const std::vector<std::string>&)> parse_func) {
  if (num_threads == 0) {
    streamRows(in, parse_func);
    return;
  }
  if (in.bad() || in.fail()) {
    throw std::runtime_error("Bad input");
  }
  std::istreambuf_iterator<char> it(in);
  pH::pool<detail::processFlat> thread_pool(num_threads);
  while (it != detail::EOCSVF) {
    thread_pool.emplace(detail::readCsvRow(it), parse_func);
  }
}

inline void streamRowsThreaded(const std::string& filename, size_t num_threads, std::function<void(const std::vector<std::string>&)> parse_func) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  streamRowsThreaded(in, num_threads, parse_func);
}

}  // namespace csv

}  // namespace pH
