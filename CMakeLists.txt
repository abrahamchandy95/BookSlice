
cmake_minimum_required(VERSION 3.20)
project(BookSlice VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(BSONCXX  REQUIRED libbsoncxx)
pkg_check_modules(MONGOCXX REQUIRED libmongocxx)
pkg_check_modules(MUPDF REQUIRED mupdf)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_executable(bookslice ${SRC_FILES})

target_include_directories(bookslice PRIVATE
    include
    ${BSONCXX_INCLUDE_DIRS}
    ${MONGOCXX_INCLUDE_DIRS}
    ${MUPDF_INCLUDE_DIRS}  
)
target_compile_options(bookslice PRIVATE -Wall -Wextra -pedantic)
target_link_libraries(bookslice PRIVATE
    ${BSONCXX_LIBRARIES}
    ${MONGOCXX_LIBRARIES}
    ${MUPDF_LIBRARIES}  
)
install(TARGETS bookslice RUNTIME DESTINATION bin)

