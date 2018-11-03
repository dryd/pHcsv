#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <fstream>
#include <algorithm>

namespace pHcsv {

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
            result.push_back('"');
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

inline std::vector<std::string> readCsvRow(std::istreambuf_iterator<char>& it) {
  std::vector<std::string> result;
  bool new_row = false;
  while (it != EOCSVF && !new_row) {
    result.push_back(readCsvField(it, new_row));
  }
  return result;
}

void writeCsvRow(std::ostreambuf_iterator<char>& it, const std::vector<std::string>& row) {
  for (size_t i = 0; i < row.size(); i++) {
    const auto& field = row.at(i);
    bool escape = false;
    for (char c : field) {
      if (c == ',' || c == '"') {
        escape = true;
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

template<typename T>
inline T convert(const std::string& str) {
  return T(str);  // hope it's constructible with a string...
}

template <>
inline std::string convert(const std::string& str) {
  return str;
}

template <>
inline double convert(const std::string& str) {
  return std::stod(str);
}

template <>
inline float convert(const std::string& str) {
  return std::stof(str);
}

template <>
inline int convert(const std::string& str) {
  return std::stoi(str);
}

template <>
inline size_t convert(const std::string& str) {
  return static_cast<size_t>(std::stoul(str));
}

}  // namespace detail

class dynamic {
 public:
  dynamic(std::istream& in, bool has_header) : header_(), data_() {
    readStream(in, has_header);
  }

  dynamic(const std::string& filename, bool has_header) : header_(), data_() {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    readStream(in, has_header);
  }

  void write(std::ostream& out, bool with_header) {
    writeStream(out, with_header);
  }

  void write(const std::string& filename, bool with_header) {
    std::ofstream out(filename, std::ios::out | std::ios::binary);
    writeStream(out, with_header);
  }

  inline size_t header_index(const std::string& column) const {
    for (size_t i = 0; i < header_.size(); i++) {
      if (header_.at(i) == column) {
        return i;
      }
    }
    throw std::runtime_error("Unrecognized column " + column);
  }

  inline size_t size() const {
    return data_.size();
  }

  inline std::string& at(size_t row, const std::string& column) {
    rangeCheck(row);
    return data_[row][header_index(column)];
  }

  inline const std::string& at(size_t row, const std::string& column) const {
    rangeCheck(row);
    return data_[row][header_index(column)];
  }

  inline std::string& at(size_t row, size_t column) {
    rangeCheck(row, column);
    return data_[row][column];
  }

  inline const std::string& at(size_t row, size_t column) const {
    rangeCheck(row, column);
    return data_[row][column];
  }

  template <typename T = std::string>
  inline T get(size_t row, const std::string& column) const {
    return detail::convert<T>(at(row, column));
  }

  template <typename T = std::string>
  inline T get(size_t row, size_t column) const {
    return detail::convert<T>(at(row, column));
  }

  bool operator==(const dynamic& other) const {
    return header_ == other.header_ && data_ == other.data_;
  }

  bool operator!=(const dynamic& other) const {
    return !(*this == other);
  }

 private:
  void readStream(std::istream& in, bool has_header) {
    if (in.bad() || in.fail()) {
      throw std::runtime_error("Bad input");
    }
    std::istreambuf_iterator<char> it(in);
    if (has_header && it != detail::EOCSVF) {
      header_ = detail::readCsvRow(it);
    }
    while (it != detail::EOCSVF) {
      std::vector<std::string> row = detail::readCsvRow(it);
      while (row.size() < header_.size()) {
        row.emplace_back();
      }
      data_.push_back(std::move(row));
    }
  }

  void writeStream(std::ostream& out, bool with_header) {
    if (out.bad() || out.fail()) {
      throw std::runtime_error("Bad output file");
    }
    std::ostreambuf_iterator<char> it(out);
    if (with_header) {
      detail::writeCsvRow(it, header_);
      it = '\n';
    }
    for (size_t i = 0; i < data_.size(); i++) {
      detail::writeCsvRow(it, data_.at(i));
      if (i != data_.size() - 1) {
        it = '\n';
      }
    }
  }

  inline void rangeCheck(size_t row) const {
    if (row >= size()) {
      throw std::runtime_error("Row " + std::to_string(row) + " out of bounds");
    }
  }

  inline void rangeCheck(size_t row, size_t col) const {
    rangeCheck(row);
    if (col >= data_[row].size()) {
      throw std::runtime_error("Column " + std::to_string(col) + " out of bounds");
    }
  }

  std::vector<std::string> header_;
  std::vector<std::vector<std::string>> data_;
};

}  // namespace pHcsv
