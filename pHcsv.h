#pragma once

#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <fstream>
#include <algorithm>
#include <functional>

namespace pH {

namespace csv {

namespace detail {

static const std::istreambuf_iterator<char> EOCSVF;

inline std::string readCsvField(std::istreambuf_iterator<char>& it, bool& new_row) {
  std::string result;
  new_row = false;
  if (it == EOCSVF) {
    return result;
  }
  if (*it == '"') {
    it++;
    bool quote = false;
    while (it != EOCSVF) {
      char c = *it;
      it++;
      if (quote) {
        switch (c) {
          case '\n':
            new_row = true;
            return result;
          case ',':
            return result;
          case '"':
            result.push_back(c);
            quote = false;
            break;
          default:
            result.push_back('"');
            result.push_back(c);
            quote = false;
            break;
          }
        } else {
          switch (c) {
            case '"':
              quote = true;
              break;
            default:
              result.push_back(c);
              break;
          }
        }
    }
  } else {
    bool quote = false;
    while (it != EOCSVF) {
      char c = *it;
      it++;
      if (quote) {
        switch (c) {
          case '\n':
            new_row = true;
            return result;
          case ',':
            return result;
          case '"':
            quote = false;
            break;
          default:
            result.push_back(c);
            quote = false;
            break;
        }
      } else {
        switch (c) {
          case '\n':
            new_row = true;
            return result;
          case ',':
            return result;
          case '"':
            result.push_back('"');
            quote = true;
            break;
          default:
            result.push_back(c);
            break;
        }
      }
    }
  }
  return result;
}

inline std::vector<std::string> readCsvRow(std::istreambuf_iterator<char>& it, size_t reserve = 0) {
  std::vector<std::string> result;
  result.reserve(reserve);
  bool new_row = false;
  size_t i = 0;
  while (it != EOCSVF && !new_row) {
    result.push_back(readCsvField(it, new_row));
    i++;
  }
  for (; i < reserve; i++) {
    result.emplace_back();
  }
  return result;
}

void readStream(std::istream& in, std::vector<std::vector<std::string>>& data, std::vector<std::string>* header = nullptr) {
  if (in.bad() || in.fail()) {
    throw std::runtime_error("Bad input");
  }
  std::istreambuf_iterator<char> it(in);
  size_t header_size = 0;
  if (header != nullptr && it != detail::EOCSVF) {
    *header = detail::readCsvRow(it);
    header_size = header->size();
  }
  while (it != detail::EOCSVF) {
    data.push_back(detail::readCsvRow(it, header_size));
  }
}

void writeCsvRow(std::ostreambuf_iterator<char>& it, const std::vector<std::string>& row) {
  for (size_t i = 0; i < row.size(); i++) {
    const auto& field = row.at(i);
    bool escape = false;
    for (char c : field) {
      if (c == ',' || c == '"') {
        escape = true;
        break;
      }
    }
    if (escape) {
      it = '"';
    }
    for (char c : field) {
      switch(c) {
        case '"':
          it = '"';
          it = '"';
          break;
        default:
          it = c;
          break;
      }
    }
    if (escape) {
      it = '"';
    }
    if (i != row.size() - 1) {
      it = ',';
    }
  }
}

void writeStream(std::ostream& out, const std::vector<std::vector<std::string>>& data, const std::vector<std::string>* header = nullptr) {
  if (out.bad() || out.fail()) {
    throw std::runtime_error("Bad output");
  }
  std::ostreambuf_iterator<char> it(out);
  if (header != nullptr) {
    detail::writeCsvRow(it, *header);
    it = '\n';
  }
  for (size_t i = 0; i < data.size(); i++) {
    detail::writeCsvRow(it, data.at(i));
    if (i != data.size() - 1) {
      it = '\n';
    }
  }
}

template<typename T>
inline T convert(const std::string& str) {
  return T(str);  // hope it's constructible with a string...
}
template <> inline std::string convert(const std::string& str) { return str; }
template <> inline double convert(const std::string& str) { return std::stod(str); }
template <> inline float convert(const std::string& str) { return std::stof(str); }
template <> inline int convert(const std::string& str) { return std::stoi(str); }
template <> inline size_t convert(const std::string& str) { return static_cast<size_t>(std::stoul(str)); }

}  // namespace detail

class flat {
 public:
  flat() : data_(), columns_(0) {}

  flat(std::istream& in) : data_(), columns_(0) {
    read(in);
  }

  flat(const std::string& filename) : data_(), columns_(0) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    read(in);
  }

  virtual void write(std::ostream& out) const {
    detail::writeStream(out, data_);
  }

  virtual void write(const std::string& filename) const {
    std::ofstream out(filename, std::ios::out | std::ios::binary);
    detail::writeStream(out, data_);
  }

  inline size_t rows() const {
    return data_.size();
  }

  virtual inline size_t columns() const {
    return columns_;
  }

  inline size_t columns(size_t row) const {
    return data_.at(row).size();
  }

  inline std::string& at(size_t row, size_t column) {
    return data_.at(row).at(column);
  }

  inline const std::string& at(size_t row, size_t column) const {
    return data_.at(row).at(column);
  }

  void emplaceRow(size_t columns) {
    data_.emplace_back(columns);
  }

  virtual void emplaceRow() {
    data_.emplace_back(columns_);
  }

  virtual void resizeColumns(size_t size) {
    columns_ = size;
    for (auto& row : data_) {
      row.resize(size);
    }
  }

  template <typename T = std::string>
  inline T get(size_t row, size_t column) const {
    return detail::convert<T>(data_.at(row).at(column));
  }

  bool operator==(const flat& other) const {
    return data_ == other.data_;
  }

  bool operator!=(const flat& other) const {
    return !(*this == other);
  }

  virtual ~flat() = default;

 protected:
  std::vector<std::vector<std::string>> data_;

 private:
  void read(std::istream& in) {
    detail::readStream(in, data_);
    for (const auto& row : data_) {
      columns_ = std::max(columns_, row.size());
    }
  }

  size_t columns_;
};

class mapped_row {
 public:
  mapped_row(const std::vector<std::string>& header, const std::vector<std::string>& data) : header_(header), data_(data) {}
  mapped_row() = delete;
  mapped_row(const mapped_row& other) = delete;
  mapped_row(mapped_row&& other) = delete;
  mapped_row& operator=(const mapped_row& other) = delete;
  mapped_row& operator=(mapped_row&& other) = delete;

  inline size_t size() const {
    return data_.size();
  }

  inline const std::string& at(const std::string& column) const {
    return data_[headerIndex(column)];
  }

  inline const std::string& at(size_t column) const {
    if (column >= data_.size()) {
      throw std::runtime_error("Column " + std::to_string(column) + " out of bounds");
    }
    return data_[column];
  }

  template <typename T = std::string>
  inline T get(const std::string& column) const {
    return detail::convert<T>(at(column));
  }

  template <typename T = std::string>
  inline T get(size_t column) const {
    return detail::convert<T>(at(column));
  }

  inline const std::vector<std::string>& data() const {
    return data_;
  }

 private:
  inline size_t headerIndex(const std::string& column) const {
    for (size_t i = 0; i < header_.size(); i++) {
      if (header_.at(i) == column) {
        return i;
      }
    }
    throw std::runtime_error("Unrecognized column " + column);
  }

  const std::vector<std::string>& header_;
  const std::vector<std::string>& data_;
};

class mapped : public flat {
 public:
  mapped(std::istream& in) : flat(), header_() {
    detail::readStream(in, data_, &header_);
  }

  mapped(const std::string& filename) : flat(), header_() {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    detail::readStream(in, data_, &header_);
  }

  void write(std::ostream& out) const override {
    detail::writeStream(out, data_, &header_);
  }

  void write(const std::string& filename) const override {
    std::ofstream out(filename, std::ios::out | std::ios::binary);
    detail::writeStream(out, data_, &header_);
  }

  void write(std::ostream& out, bool ignore_header) const {
    detail::writeStream(out, data_, ignore_header ? nullptr : &header_);
  }

  void write(const std::string& filename, bool ignore_header) const {
    std::ofstream out(filename, std::ios::out | std::ios::binary);
    detail::writeStream(out, data_, ignore_header ? nullptr : &header_);
  }

  inline size_t headerIndex(const std::string& column) const {
    for (size_t i = 0; i < header_.size(); i++) {
      if (header_.at(i) == column) {
        return i;
      }
    }
    throw std::runtime_error("Unrecognized column " + column);
  }

  using flat::at;

  inline std::string& at(size_t row, const std::string& column) {
    return data_.at(row).at(headerIndex(column));
  }

  inline const std::string& at(size_t row, const std::string& column) const {
    return data_.at(row).at(headerIndex(column));
  }

  void emplaceRow() override {
    data_.emplace_back(header_.size(), "");
  }

  void resizeColumns(size_t size) override {
    header_.resize(size);
    flat::resizeColumns(size);
  }

  inline size_t columns() const override {
    return header_.size();
  }

  inline void emplaceColumn(const std::string& column) {
    if (std::find(header_.begin(), header_.end(), column) == header_.end()) {
      header_.push_back(column);
      for (auto& row : data_) {
        row.emplace_back();
      }
    }
  }

  using flat::get;

  template <typename T = std::string>
  inline T get(size_t row, const std::string& column) const {
    return mapped_row(header_, data_.at(row)).get<T>(column);
  }

  bool operator==(const mapped& other) const {
    return header_ == other.header_ && flat::operator==(other);
  }

  bool operator!=(const mapped& other) const {
    return !(*this == other);
  }

 private:
  std::vector<std::string> header_;
};

void streamRows(std::istream& in, std::function<void(const mapped_row&)> parse_func) {
  if (in.bad() || in.fail()) {
    throw std::runtime_error("Bad input");
  }
  std::istreambuf_iterator<char> it(in);
  std::vector<std::string> header = detail::readCsvRow(it);
  while (it != detail::EOCSVF) {
    parse_func(mapped_row(header, detail::readCsvRow(it, header.size())));
  }
}

inline void streamRows(const std::string& filename, std::function<void(const mapped_row&)> parse_func) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  streamRows(in, parse_func);
}

void streamRows(std::istream& in, std::function<void(const std::vector<std::string>&)> parse_func) {
  if (in.bad() || in.fail()) {
    throw std::runtime_error("Bad input");
  }
  std::istreambuf_iterator<char> it(in);
  while (it != detail::EOCSVF) {
    parse_func(detail::readCsvRow(it));
  }
}

inline void streamRows(const std::string& filename, std::function<void(const std::vector<std::string>&)> parse_func) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  streamRows(in, parse_func);
}

}  // namespace csv

}  // namespace pH
