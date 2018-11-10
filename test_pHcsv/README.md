pH::csv
=======

pH::csv is a single-header C++11 library for parsing CSV files. The goal is to create a fast CSV parser with an intuitive interface.

There are two classes defined. pH::csv::mapped is used for CSV files with headers, while pH::csv::flat is used for CSV files without headers. The data fields are saved as strings, and for pH::csv::mapped the header is saved separately and used to index the rows.

Additionally it's possible to stream a CSV file into your own custom data structure using the function pH::csv::streamTo.

We will demonstrate how to use the library with the csv file from wikipedia (https://en.wikipedia.org/wiki/Comma-separated_values):

```
Year,Make,Model,Description,Price
1997,Ford,E350,"ac, abs, moon",3000.00
1999,Chevy,"Venture ""Extended Edition""","",4900.00
1999,Chevy,"Venture ""Extended Edition, Very Large""",,5000.00
1996,Jeep,Grand Cherokee,"MUST SELL!
air, moon roof, loaded",4799.00
```

pH::csv::mapped and pH::csv::flat
---------------------------------

These classes allow dynamic CSV data to be read and stored as std::strings. The data can also be modified and written to a new file.

```cpp
#include <pHcsv.h>
#include <iostream>

int main() {
  // Use pH::csv::mapped for reading files with headers
  pH::csv::mapped cars("test_data/wiki.csv");

  // Sizes
  std::cout << cars.rows() << "x" << cars.columns() << std::endl; // 4x5

  // Access data by header or index
  std::cout << cars.at(1, "Model") << std::endl; // Venture "Extended Edition"
  std::cout << cars.at(1, 1) << std::endl; // Chevy

  // Automatic conversion of standard types
  int year = cars.get<int>(0, "Year");

  // Modifiable reference to data
  cars.at(1, "Make") = "Chevrolet";

  // Add data
  cars.emplaceRow();
  cars.at(4, "Make") = "BMW";
  cars.at(4, "Model") = "M3";
  cars.at(4, "Price") = "5500.0";
  cars.emplaceColumn("Extras");
  cars.at(4, "Extras") = "blinker fluid";

  // Write data to file (second argument = skip_header)
  cars.write("saved_flat.csv", true);

  // Use pH::csv::flat for reading files without header
  pH::csv::flat flat_cars("saved_flat.csv");

  // Can only access flat data by index
  std::cout << flat_cars.at(1, 1) << std::endl; // Chevrolet
  std::cout << flat_cars.get(2, 5) << std::endl; //
  std::cout << flat_cars.get<float>(4, 4) << std::endl; // 5500.0

  // pH::csv::mapped inherits from pH::csv::flat
  const pH::csv::flat& mapped_base = cars;
  std::cout << (mapped_base == flat_cars) << std::endl; // true
}
```

pH::csv::streamRows
-------------------

For many applications, it's likely that users wants to use their own data structure to store data from a CSV file to be able to process it later. While it's possible to first use pH::csv::mapped or pH::csv::flat and then convert the resulting data, it is more memory efficient and performant to stream the CSV rows directly to the data structure.

```cpp
#include <pHcsv.h>
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
```
