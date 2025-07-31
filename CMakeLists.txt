
cmake_minimum_required(VERSION 3.20)

project(BookSlice VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

# Poppler
pkg_check_modules(POPPLER_CPP REQUIRED poppler-cpp)

# MongoDB C++ driver via pkg-config
pkg_check_modules(BSONCXX  REQUIRED libbsoncxx)
pkg_check_modules(MONGOCXX REQUIRED libmongocxx)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_executable(bookslice ${SRC_FILES})

target_include_directories(bookslice PRIVATE
    include
    ${POPPLER_CPP_INCLUDE_DIRS}
    ${BSONCXX_INCLUDE_DIRS}
    ${MONGOCXX_INCLUDE_DIRS}
)

target_link_libraries(bookslice PRIVATE
    ${POPPLER_CPP_LIBRARIES}
    ${BSONCXX_LIBRARIES}
    ${MONGOCXX_LIBRARIES}
)

install(TARGETS bookslice RUNTIME DESTINATION bin)


