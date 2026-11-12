cmake_minimum_required(VERSION 3.10)
project(GlassMarketApp)
set(CMAKE_CXX_STANDARD 20)

find_package(libpqxx CONFIG REQUIRED)
find_package(TgBot CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)

add_executable(backend_app main.cpp)
target_link_libraries(backend_app PRIVATE libpqxx::pqxx TgBot::TgBot cpr::cpr)