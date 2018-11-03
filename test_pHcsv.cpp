#include "pHcsv.h"

#include <assert.h>

template<typename T>
inline std::string toString(const T& val) {
  return std::to_string(val);
}

template<>
inline std::string toString(const std::string& val) {
  return val;
}

#define ASSERT_EQ(expr, expected) if ((expr) != (expected)) { printf("Assert failed at line %d:\n  %s != %s\n", __LINE__, toString(expr).c_str(), #expected); return 1; }

int test_dynamic_wiki() {
  pHcsv::dynamic data("../dcsv/test_data/wiki.csv", true);

  // Test get() and at()
  ASSERT_EQ(data.at(0, "Extras"), "steering \"wheel\"");
  ASSERT_EQ(data.at(0, "Extras"), data.at(0, 5));
  ASSERT_EQ(data.at(0, "Extras"), data.get(0, "Extras"));
  ASSERT_EQ(data.at(0, "Extras"), data.get(0, 5));
  ASSERT_EQ(data.at(1, "Extras"), "wheels and \"frame\"");
  ASSERT_EQ(data.at(2, "Extras"), "LED-\"lights\"");
  ASSERT_EQ(data.at(2, "Model"), "Venture \"Extended Edition, Very Large\"");
  ASSERT_EQ(data.at(3, "Description"), "MUST SELL!\nair, moon \"\"roof\"\", loaded");

  ASSERT_EQ(data.at(3, "Extras"), "");
  data.at(3, "Extras") = "new data";
  ASSERT_EQ(data.at(3, "Extras"), "new data");

  ASSERT_EQ(data.get<int>(0, "Year"), 1997);
  ASSERT_EQ(data.get<size_t>(3, "Year"), 1996);
  ASSERT_EQ(data.get<float>(3, "Price"), 4799.0f);
  ASSERT_EQ(data.get<double>(1, "Price"), 4900.0);

  // Test write()
  data.write("../dcsv/test_data/wiki_written.csv", true);
  pHcsv::dynamic written_data("../dcsv/test_data/wiki_written.csv", true);
  if (data != written_data) {
    printf("Written data does not match read data\n");
    return 1;
  }
  std::remove("../dcsv/test_data/wiki_written.csv");

  return 0;
}

int main() {
  return test_dynamic_wiki();
}
