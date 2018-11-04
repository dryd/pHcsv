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

  // Test add data
  data.addRow();
  data.at(4, "Price") = "4200.00";
  ASSERT_EQ(data.get<double>(4, "Price"), 4200.0);
  data.addHeader("CC");
  ASSERT_EQ(data.get<std::string>(3, "CC"), "");
  data.at(4, "CC") = "3997";
  ASSERT_EQ(data.get<double>(4, "CC"), 3997.0);

  // Test write()
  data.write("../dcsv/test_data/wiki_written.csv", true);
  pHcsv::dynamic written_data("../dcsv/test_data/wiki_written.csv", true);
  if (data != written_data) {
    printf("Written data does not match data\n");
    return 1;
  }
  std::remove("../dcsv/test_data/wiki_written.csv");

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

int assert_typed_wiki(pHcsv::typed<Car>& data) {
  // Test get() and at()
  ASSERT_EQ(data.at(0).extras, "steering \"wheel\"");
  ASSERT_EQ(data.at(1).extras, "wheels and \"frame\"");
  ASSERT_EQ(data.at(2).extras, "LED-\"lights\"");
  ASSERT_EQ(data.at(2).model, "Venture \"Extended Edition, Very Large\"");
  ASSERT_EQ(data.at(3).description, "MUST SELL!\nair, moon \"\"roof\"\", loaded");
  ASSERT_EQ(data.at(0).year, 1997);
  ASSERT_EQ(data.at(3).price, 4799.0);

  ASSERT_EQ(data.at(3).extras, "");
  data.at(3).extras = "new data";
  ASSERT_EQ(data.at(3).extras, "new data");

  // Test add data
  data.emplace_back();
  data.at(4).price = 4200.00;
  ASSERT_EQ(data.at(4).price, 4200.0);

  return 0;
}

Car parseCar(const std::map<std::string, std::string>& row) {
  Car car;
  car.year = std::stoi(row.at("Year"));
  car.make = row.at("Make");
  car.model = row.at("Model");
  car.description = row.at("Description");
  car.price = std::stod(row.at("Price"));
  car.extras = row.at("Extras");
  return car;
}

int test_typed_wiki_header() {
  pHcsv::typed<Car> data("../dcsv/test_data/wiki.csv", parseCar);
  int status = assert_typed_wiki(data);
  data.write("../dcsv/test_data/wiki_written.csv", [] (const Car& car) {
    std::map<std::string, std::string> result;
    result["Year"] = std::to_string(car.year);
    result["Price"] = std::to_string(car.price);
    result["Make"] = car.make;
    result["Model"] = car.model;
    result["Description"] = car.description;
    result["Extras"] = car.extras;
    return result;
  });
  pHcsv::typed<Car> written_data("../dcsv/test_data/wiki_written.csv", parseCar);
  ASSERT_EQ(data.at(0).extras, written_data.at(0).extras);
  std::remove("../dcsv/test_data/wiki_written.csv");

  return status;
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

int test_typed_wiki_no_header() {
  pHcsv::typed<Car> data("../dcsv/test_data/wiki_no_header.csv", parseCarFromVec);
  int status = assert_typed_wiki(data);
  data.write("../dcsv/test_data/wiki_written.csv", [] (const Car& car) {
    return std::vector<std::string>{std::to_string(car.year), car.make, car.model, car.description, std::to_string(car.price), car.extras};
  });
  pHcsv::typed<Car> written_data("../dcsv/test_data/wiki_written.csv", parseCarFromVec);
  ASSERT_EQ(data.at(0).extras, written_data.at(0).extras);
  std::remove("../dcsv/test_data/wiki_written.csv");

  return status;
}

int main() {
  return test_dynamic_wiki() + test_typed_wiki_header() + test_typed_wiki_no_header();
}
