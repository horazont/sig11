project(sig11)
cmake_minimum_required(VERSION 3.0)

set(SIG11_SRCS
   src/sig11.cpp
)
set(SIG11_HEADERS
   include/sig11/sig11.hpp
)

find_package(Threads REQUIRED)

add_library(sig11 STATIC ${SIG11_SRCS})
set_property(TARGET sig11 PROPERTY CXX_STANDARD 14)
set_property(TARGET sig11 PROPERTY CXX_STANDARD_REQUIRED ON)
target_compile_options(sig11 PRIVATE -Wall -Wextra)
target_compile_options(sig11 PRIVATE $<$<CONFIG:DEBUG>:-ggdb -O2>)
target_compile_options(sig11 PRIVATE $<$<CONFIG:RELEASE>:-O3>)
target_include_directories(sig11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_custom_target(dummy_sig11_files SOURCES ${SIG11_HEADERS})

set(SIG11_TEST_SRCS
   tests/src/main.cpp
   tests/src/signal.cpp
   tests/src/connection.cpp
   tests/src/connection_guard.cpp
)

add_executable(sig11_tests ${SIG11_TEST_SRCS})
target_include_directories(sig11_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests/Catch/include)
target_link_libraries(sig11_tests sig11 ${CMAKE_THREAD_LIBS_INIT})
set_property(TARGET sig11_tests PROPERTY CXX_STANDARD 14)
set_property(TARGET sig11_tests PROPERTY CXX_STANDARD_REQUIRED ON)
target_compile_options(sig11_tests PRIVATE -Wall -Wextra)
target_compile_options(sig11_tests PRIVATE $<$<CONFIG:DEBUG>:-ggdb -O2>)
target_compile_options(sig11_tests PRIVATE $<$<CONFIG:RELEASE>:-O3>)
