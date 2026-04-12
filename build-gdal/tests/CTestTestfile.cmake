# CMake generated Testfile for 
# Source directory: E:/projects/enc/navscene-sdk/tests
# Build directory: E:/projects/enc/navscene-sdk/build-gdal/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[navscene-smoke]=] "E:/projects/enc/navscene-sdk/build-gdal/tests/Debug/navscene-smoke.exe")
  set_tests_properties([=[navscene-smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "E:/projects/enc/navscene-sdk/tests/CMakeLists.txt;12;add_test;E:/projects/enc/navscene-sdk/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[navscene-smoke]=] "E:/projects/enc/navscene-sdk/build-gdal/tests/Release/navscene-smoke.exe")
  set_tests_properties([=[navscene-smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "E:/projects/enc/navscene-sdk/tests/CMakeLists.txt;12;add_test;E:/projects/enc/navscene-sdk/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[navscene-smoke]=] "E:/projects/enc/navscene-sdk/build-gdal/tests/MinSizeRel/navscene-smoke.exe")
  set_tests_properties([=[navscene-smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "E:/projects/enc/navscene-sdk/tests/CMakeLists.txt;12;add_test;E:/projects/enc/navscene-sdk/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[navscene-smoke]=] "E:/projects/enc/navscene-sdk/build-gdal/tests/RelWithDebInfo/navscene-smoke.exe")
  set_tests_properties([=[navscene-smoke]=] PROPERTIES  _BACKTRACE_TRIPLES "E:/projects/enc/navscene-sdk/tests/CMakeLists.txt;12;add_test;E:/projects/enc/navscene-sdk/tests/CMakeLists.txt;0;")
else()
  add_test([=[navscene-smoke]=] NOT_AVAILABLE)
endif()
