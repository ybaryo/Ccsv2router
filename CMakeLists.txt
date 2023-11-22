cmake_minimum_required(VERSION 3.18)
project(csv2qr)
# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_executable(csv2qr ${CMAKE_SOURCE_DIR}/cpp_main/CdrToQRouter.cpp ${CMAKE_SOURCE_DIR}/cpp_csv2qr/CdrToQRouterLib.cpp)


#add_subdirectory(${CMAKE_SOURCE_DIR}/cpp_csv2qr)
include_directories(${CMAKE_SOURCE_DIR}/include)

target_link_directories(csv2qr PUBLIC ${CMAKE_SOURCE_DIR}/precompiled_libs)
target_link_libraries(csv2qr zlib)

target_compile_features(csv2qr PRIVATE cxx_std_17)