# cmake --build . --config Release

cmake_minimum_required(VERSION 3.20)

project(jsini VERSION 1.0)

add_library(jsini STATIC jsa.c  jsb.c  jsc.c  jsh.c  jsini.c  jsini_ini.c  jsini_json.c  jsx.c)

