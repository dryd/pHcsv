#include "pHcsv.h"

template<typename T>
inline std::string toString(const T& val) {
  return std::to_string(val);
}

template<>
inline std::string toString(const std::string& val) {
  return val;
}

#define ASSERT_EQ(expr, expected) if ((expr) != (expected)) { printf("Assert failed at line %d:\n  %s != %s\n", __LINE__, toString(expr).c_str(), #expected); return 1; }

int test_mapped_wiki() {
  pHcsv::mapped data("../dcsv/test_data/wiki.csv");

  // Test get() and at()
  ASSERT_EQ(data.rows(), 4);
  ASSERT_EQ(data.columns(), 6);
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

  // Test add data
  data.emplaceRow();
  ASSERT_EQ(data.rows(), 5);
  data.at(4, "Price") = "4200.00";
  ASSERT_EQ(data.get<double>(4, "Price"), 4200.0);
  data.emplaceColumn("CC");
  ASSERT_EQ(data.columns(), 7);
  ASSERT_EQ(data.get<std::string>(3, "CC"), "");
  data.at(4, "CC") = "3997";
  ASSERT_EQ(data.get<double>(4, "CC"), 3997.0);

  // Test write()
  data.write("../dcsv/test_data/wiki_written.csv");
  pHcsv::mapped written_data("../dcsv/test_data/wiki_written.csv");
  if (data != written_data) {
    printf("Written data does not match data\n");
    return 1;
  }
  std::remove("../dcsv/test_data/wiki_written.csv");

  return 0;
}

int test_flat_wiki() {
  pHcsv::flat data("../dcsv/test_data/wiki_no_header.csv");

  // Test get() and at()
  ASSERT_EQ(data.rows(), 4);
  ASSERT_EQ(data.columns(), 6);
  ASSERT_EQ(data.at(0, 5), "steering \"wheel\"");
  ASSERT_EQ(data.get(0, 5), data.at(0, 5));

  ASSERT_EQ(data.get<int>(0, 0), 1997);
  ASSERT_EQ(data.get<size_t>(3, 0), 1996);
  ASSERT_EQ(data.get<float>(3, 4), 4799.0f);
  ASSERT_EQ(data.get<double>(1, 4), 4900.0);

  // Test add data
  data.emplaceRow();
  ASSERT_EQ(data.rows(), 5);
  data.at(4, 4) = "4200.00";
  ASSERT_EQ(data.get<double>(4, 4), 4200.0);
  data.resizeColumns(7);
  ASSERT_EQ(data.columns(), 7);
  ASSERT_EQ(data.get<std::string>(3, 6), "");
  data.at(4, 6) = "3997";
  ASSERT_EQ(data.get<double>(4, 6), 3997.0);

  // Test write()
  data.write("../dcsv/test_data/wiki_written.csv");
  pHcsv::flat written_data("../dcsv/test_data/wiki_written.csv");
  if (data != written_data) {
    printf("Written data does not match data\n");
    return 1;
  }
  //std::remove("../dcsv/test_data/wiki_written.csv");

  return 0;
}

struct Car {
  int year;
  std::string make;
  std::string model;
  std::string description;
  double price;
  std::string extras;
};

Car parseCar(const pHcsv::mapped_row& row) {
  Car car;
  car.year = row.get<int>("Year");
  car.make = row.at("Make");
  car.model = row.at("Model");
  car.description = row.at("Description");
  car.price = row.get<double>(4);
  car.extras = row.at("Extras");
  return car;
}

Car parseCarFromVec(const std::vector<std::string>& row) {
  Car car;
  car.year = std::stoi(row.at(0));
  car.make = row.at(1);
  car.model = row.at(2);
  car.description = row.at(3);
  car.price = std::stod(row.at(4));
  if (row.size() <= 5) {
    car.extras = "";
  } else {
    car.extras = row.at(5);
  }
  return car;
}

int test_streaming() {
  std::map<int, std::vector<Car>> cheap_cars_by_year;
  pHcsv::streamRows("../dcsv/test_data/wiki.csv", [&cheap_cars_by_year] (const pHcsv::mapped_row& row) {
    if (std::stod(row.at("Price")) < 4800.0) {
      cheap_cars_by_year[std::stoi(row.at("Year"))].push_back(parseCar(row));
    }
  });
  ASSERT_EQ(cheap_cars_by_year.size(), 2);
  ASSERT_EQ(cheap_cars_by_year.at(1997).front().model, "E350");

  cheap_cars_by_year.clear();
  pHcsv::streamRows("../dcsv/test_data/wiki_no_header.csv", [&cheap_cars_by_year] (const std::vector<std::string>& row) {
    if (std::stod(row.at(4)) < 4800.0) {
      cheap_cars_by_year[std::stoi(row.at(0))].push_back(parseCarFromVec(row));
    }
  });
  ASSERT_EQ(cheap_cars_by_year.size(), 2);
  ASSERT_EQ(cheap_cars_by_year.at(1997).front().model, "E350");

  return 0;
}

int main() {
  return test_mapped_wiki() + test_flat_wiki() + test_streaming();
}
