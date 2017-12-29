cmake_minimum_required(VERSION 3.9)
project(MTA_TEA_Encoder)

set(CMAKE_CXX_STANDARD 14)

add_executable(MTA_TEA_Encoder main.cpp md5.cpp base64.cpp)