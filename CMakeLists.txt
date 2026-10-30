cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE "C:/Artiom/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(GlassMarketApp)

set(CMAKE_CXX_STANDARD 20)

find_package(libpqxx CONFIG REQUIRED)
add_executable(backend_app main.cpp)
target_link_libraries(backend_app PRIVATE libpqxx::pqxx)