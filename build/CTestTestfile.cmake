# CMake generated Testfile for 
# Source directory: D:/STUDY/Projects/lithium-matcher
# Build directory: D:/STUDY/Projects/lithium-matcher/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(LithiumTests "D:/STUDY/Projects/lithium-matcher/build/Debug/tests.exe")
  set_tests_properties(LithiumTests PROPERTIES  _BACKTRACE_TRIPLES "D:/STUDY/Projects/lithium-matcher/CMakeLists.txt;41;add_test;D:/STUDY/Projects/lithium-matcher/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(LithiumTests "D:/STUDY/Projects/lithium-matcher/build/Release/tests.exe")
  set_tests_properties(LithiumTests PROPERTIES  _BACKTRACE_TRIPLES "D:/STUDY/Projects/lithium-matcher/CMakeLists.txt;41;add_test;D:/STUDY/Projects/lithium-matcher/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(LithiumTests "D:/STUDY/Projects/lithium-matcher/build/MinSizeRel/tests.exe")
  set_tests_properties(LithiumTests PROPERTIES  _BACKTRACE_TRIPLES "D:/STUDY/Projects/lithium-matcher/CMakeLists.txt;41;add_test;D:/STUDY/Projects/lithium-matcher/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(LithiumTests "D:/STUDY/Projects/lithium-matcher/build/RelWithDebInfo/tests.exe")
  set_tests_properties(LithiumTests PROPERTIES  _BACKTRACE_TRIPLES "D:/STUDY/Projects/lithium-matcher/CMakeLists.txt;41;add_test;D:/STUDY/Projects/lithium-matcher/CMakeLists.txt;0;")
else()
  add_test(LithiumTests NOT_AVAILABLE)
endif()
subdirs("_deps/googletest-build")
