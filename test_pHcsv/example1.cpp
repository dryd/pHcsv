#include "../pHcsv.h"
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