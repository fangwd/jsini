cmake_minimum_required(VERSION 3.10)

project(jsini)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

add_library(libjsini STATIC
  src/jsa.c
  src/jsb.c
  src/jsc.c
  src/jsh.c
  src/jsini.c
  src/jsini_ini.c
  src/jsini_json.c
)
set_target_properties(libjsini PROPERTIES OUTPUT_NAME "jsini")
target_include_directories(libjsini
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(jsini src/main.c)
target_include_directories(jsini PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(jsini PRIVATE libjsini)

add_executable(test
    tests/jsa.c
    tests/jsh.c
    tests/utf8.c
    tests/jsl.cpp
    tests/main.cpp
)
target_link_libraries(test PRIVATE libjsini)
