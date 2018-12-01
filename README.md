practicalHeaders
===============

Some single-header C++11 libraries that I made for fun. They only depend on STL and should be compilable on most platforms (only clang and gcc tested).

- [pHcsv](test_pHcsv) is a csv parser
- [pHpool](test_pHpool) is a thread pool
- [pHcsvthread](test_pHcsvthread) uses pHpool to extend pHcsv to support multithreaded CSV parsing
- [pHad](test_pHad) is a reverse automatic differentiation library

The actual library .h-files all reside in the /src folder of the repository, with tests and examples for each library in separate subfolders.
