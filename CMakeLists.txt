project(sig11)
cmake_minimum_required(VERSION 3.0)

set(SIG11_SRCS
   src/sig11.cpp
)

add_library(sig11 STATIC ${SIG11_SRCS})
set_property(TARGET sig11 PROPERTY CXX_STANDARD 14)
set_property(TARGET sig11 PROPERTY CXX_STANDARD_REQUIRED ON)
target_compile_options(sig11 PRIVATE -Wall -Wextra)
target_compile_options(sig11 PRIVATE $<$<CONFIG:DEBUG>:-ggdb -O2>)
target_compile_options(sig11 PRIVATE $<$<CONFIG:RELEASE>:-O3>)
target_include_directories(sig11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SIG11_TEST_SRCS
   tests/src/main.cpp
   tests/src/first.cpp
)

add_executable(sig11_tests ${SIG11_TEST_SRCS})
target_include_directories(sig11_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/Catch/include)
set_property(TARGET sig11_tests PROPERTY CXX_STANDARD 14)
set_property(TARGET sig11_tests PROPERTY CXX_STANDARD_REQUIRED ON)
target_compile_options(sig11_tests PRIVATE -Wall -Wextra)
target_compile_options(sig11_tests PRIVATE $<$<CONFIG:DEBUG>:-ggdb -O2>)
target_compile_options(sig11_tests PRIVATE $<$<CONFIG:RELEASE>:-O3>)
