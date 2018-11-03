#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <iostream>

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
    int consecutive_quotes = 0;
    while (it != EOCSVF) {
      char c = *it;
      it++;
      if (consecutive_quotes) {
        switch (c) {
          case '\n':
            new_row = true;
            return result;
          case ',':
            return result;
          case '"':
            consecutive_quotes++;
            if (consecutive_quotes % 2 == 0) {
              result.push_back('"');
            }
            break;
          default:
            if (consecutive_quotes == 1) {
              result.push_back('"');
            }
            result.push_back(c);
            consecutive_quotes = 0;
            break;
          }
        } else {
          switch (c) {
            case '"':
              consecutive_quotes = 1;
              break;
            default:
              result.push_back(c);
              break;
          }
        }
    }
  } else {
    while (it != EOCSVF) {
      char c = *it;
      it++;
      switch (c) {
        case '\n':
         new_row = true;
          return result;
        case ',':
          return result;
        default:
          result.push_back(c);
          break;
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

}  // namespace detail

class Dynamic {
 public:
  Dynamic(std::istream& in, bool has_header) : indices_(), data_() {
    std::istreambuf_iterator<char> it(in);
    if (has_header && it != detail::EOCSVF) {
      std::vector<std::string> header = detail::readCsvRow(it);
      for (const auto& h : header) {
        if (indices_.count(h) == 0) {
          size_t i = indices_.size();
          indices_[h] = i;
        }
      }
    }
    while (it != detail::EOCSVF) {
      std::vector<std::string> row = detail::readCsvRow(it);
      while (row.size() < indices_.size()) {
        row.emplace_back();
      }
      data_.push_back(std::move(row));
    }
  }

  inline std::string& at(size_t row, const std::string& column) {
    if (row >= data_.size()) {
      throw std::runtime_error("Row " + std::to_string(row) + " out of bounds");
    }
    auto index_it = indices_.find(column);
    if (index_it == indices_.end()) {
      throw std::runtime_error("Unrecognized column " + column);
    }
    return data_[row][index_it->second];
  }

  inline const std::string& at(size_t row, const std::string& column) const {
    return at(row, column);
  }

  template <typename T>
  inline T get(size_t row, std::string column) const {
    return T(at(row, column));
  }

 private:
  std::map<std::string, size_t> indices_;
  std::vector<std::vector<std::string>> data_;
};



}  // namespace dcsv
