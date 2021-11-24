cmake_minimum_required(VERSION 2.9)
project(PunyCode)

set(CMAKE_CXX_STANDARD 17)

add_executable(
  PunyCode
  punycode.h
  test-punycode.cpp
)
