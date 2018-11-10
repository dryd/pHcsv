#include "../pHcsv.h"
#include <iostream>

struct Car {
  std::string make;
  std::string model;
  std::string description;
  double price;
};

int main() {
  // Custom data structure to which we want to read a CSV
  std::map<int, std::vector<Car>> luxury_cars_by_year;

  // Read file with pH::csv::streamRows
  // Assumes CSV file has header if lambda takes pH::csv::mapped_row
  // Assumes header less CSV file if lambda takes std::vector<std::string>
  pH::csv::streamRows("test_data/wiki.csv", [&luxury_cars_by_year] (const pH::csv::mapped_row& row) {
    double price = row.get<double>("Price");
    if (price > 4000.0) {
      Car car;
      car.make = row.at("Make");
      car.model = row.at("Model");
      car.description = row.at("Description");
      car.price = price;
      luxury_cars_by_year[row.get<int>("Year")].push_back(std::move(car));
    }
  });

  for (const auto& year_car : luxury_cars_by_year) {
    std::cout << year_car.first << ": " << std::endl;
    for (const Car& car : year_car.second) {
      std::cout << "  - " << car.make << ", " << car.model << ", " << car.description << ", " << car.price << std::endl;
    }
  }
  /*
  1996:
    - Jeep, Grand Cherokee, MUST SELL!
  air, moon roof, loaded, 4799
  1999:
    - Chevy, Venture "Extended Edition", , 4900
    - Chevy, Venture "Extended Edition, Very Large", , 5000
  */
}