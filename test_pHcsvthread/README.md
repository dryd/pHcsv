pH::csvthread
=============

pH::csvthread extends pH::csv with pH::pool, creating a multithreaded CSV reader.

It streams rows into a user data structure parallel to reading the actual CSV file, and the interface is very similar to that of pH::csv.

Since parsed CSV rows must be stored until processed, more memory will probably be used than when using pH::csv::streamRows.

pH::csv::streamRowsThreaded
---------------------------

```cpp
#include <pHcsvthread.h>
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
  std::mutex mut;

  // pH::csv::streamRowsThreaded works similarly to pH::csv::streamRows
  // The main thread will be used for CSV parsing
  // The second argument is number of threads to be used in data processing.
  // If the number is higher than 1, make sure to protect your data structure from race conditions.
  pH::csv::streamRowsThreaded("test_data/wiki.csv", 3, [&luxury_cars_by_year, &mut] (const pH::csv::mapped_row& row) {
    double price = row.get<double>("Price");
    if (price > 4000.0) {
      Car car;
      car.make = row.at("Make");
      car.model = row.at("Model");
      car.description = row.at("Description");
      car.price = price;
      int year = row.get<int>("Year");

      std::lock_guard<std::mutex> lock(mut);
      luxury_cars_by_year[year].push_back(std::move(car));
    }
  });
}
```
