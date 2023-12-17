cmake_minimum_required(VERSION 3.0)
project(leakFinder C CXX)

add_subdirectory(examples)

add_executable(leakfinder leakfinder.cpp)

add_library(mallocCatcher SHARED mallocCatcher.cpp)

add_library(lockCatcher SHARED lockCatcher.cpp)