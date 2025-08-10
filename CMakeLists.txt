cmake_minimum_required(VERSION 3.20)
project(BookSlice VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(BSONCXX  REQUIRED IMPORTED_TARGET libbsoncxx)
pkg_check_modules(MONGOCXX REQUIRED IMPORTED_TARGET libmongocxx)
pkg_check_modules(MUPDF    REQUIRED IMPORTED_TARGET mupdf)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_executable(bookslice ${SRC_FILES})

target_include_directories(bookslice PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(bookslice PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(bookslice PRIVATE
  PkgConfig::BSONCXX
  PkgConfig::MONGOCXX
  PkgConfig::MUPDF
)
