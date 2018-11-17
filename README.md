practicalHeaders
===============

Some single-header C++11 libraries that I made for fun. They only depend on STL and should be compilable on most platforms (only clang and gcc tested).

- [pHcsv](test_pHcsv) is a csv parser
- [pHpool](test_pHpool) is a thread pool
- [pHcsvthread](test_pHcsvthread) uses pHpool to extend pHcsv to support multithreaded CSV parsing

The actual library .h-files reside in the root folder of the repository, while tests and examples are in subfolders.
