#include "pHcsv.h"

#include <fstream>

int test_dynamic_wiki() {
  std::ifstream wiki("../dcsv/test_data/wiki.csv");
  pHcsv::Dynamic csv_data(wiki, true);
  std::cout << csv_data.at(1, "Extras") << std::endl;
  std::cout << csv_data.at(2, "Extras") << std::endl;
  std::cout << csv_data.at(3, "Extras") << std::endl;
  std::cout << csv_data.at(2, "Model") << std::endl;
  std::cout << csv_data.at(3, "Description") << std::endl;
  return 0;
}

int main() {
    test_dynamic_wiki();
    return 0;
}
