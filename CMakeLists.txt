
cmake_minimum_required(VERSION 3.20)
project(BookSlice VERSION 0.1 LANGUAGES CXX)

# Prefer target-based standard; keeps toolchains cleaner
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Collect sources
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_executable(bookslice ${SRC_FILES})
target_link_libraries(bookslice PRIVATE project_options)
target_include_directories(bookslice PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(bookslice PRIVATE -Wall -Wextra -Wpedantic)

# ---- MongoDB C++ driver (mongocxx/bsoncxx) ----
# Try CMake config packages first (Homebrew usually provides these)
find_package(mongocxx CONFIG QUIET)
find_package(bsoncxx CONFIG QUIET)

if(mongocxx_FOUND AND bsoncxx_FOUND)
  # Link against shared libs; change to *_static if you prefer static
  set(MONGO_LIBS mongo::mongocxx_shared mongo::bsoncxx_shared)
else()
  # Fallback to pkg-config
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(BSONCXX REQUIRED IMPORTED_TARGET libbsoncxx)
  pkg_check_modules(MONGOCXX REQUIRED IMPORTED_TARGET libmongocxx)
  set(MONGO_LIBS PkgConfig::MONGOCXX PkgConfig::BSONCXX)
endif()

# ---- MuPDF ----
# Prefer pkg-config if it exists
set(MUPDF_LIBS "")
set(HAVE_MUPDF_PKG FALSE)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(MUPDF QUIET IMPORTED_TARGET mupdf)
  if(TARGET PkgConfig::MUPDF)
    set(MUPDF_LIBS PkgConfig::MUPDF)
    set(HAVE_MUPDF_PKG TRUE)
  endif()
endif()

# Fallback: manual discovery (common on macOS)
if(NOT HAVE_MUPDF_PKG)
  # Library names vary by distro; try typical combos
  find_library(MUPDF_LIB         NAMES mupdf)
  find_library(MUPDF_THIRD_LIB   NAMES mupdfthird mupdf-third mupdf-thirdparty)
  find_path(MUPDF_INCLUDE_DIR    NAMES mupdf/fitz.h mupdf/pdf.h)

  if(MUPDF_LIB AND MUPDF_INCLUDE_DIR)
    add_library(mupdf_unknown INTERFACE)
    target_include_directories(mupdf_unknown INTERFACE ${MUPDF_INCLUDE_DIR})
    if(MUPDF_THIRD_LIB)
      target_link_libraries(mupdf_unknown INTERFACE ${MUPDF_LIB} ${MUPDF_THIRD_LIB})
    else()
      target_link_libraries(mupdf_unknown INTERFACE ${MUPDF_LIB})
    endif()
    set(MUPDF_LIBS mupdf_unknown)
  else()
    message(FATAL_ERROR "MuPDF not found. Install it or set MUPDF_INCLUDE_DIR / MUPDF_LIB.")
  endif()
endif()

# Link everything
target_link_libraries(bookslice PRIVATE ${MONGO_LIBS} ${MUPDF_LIBS})

# ---- macOS niceties (rpath) ----
# Helps the app find libs from Homebrew without manual DYLD_LIBRARY_PATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
if(APPLE)
  set(CMAKE_MACOSX_RPATH 1)
  # When running from the build tree:
  set(CMAKE_BUILD_RPATH "@loader_path")
  # When installed next to libs:
  set(CMAKE_INSTALL_RPATH "@loader_path/../lib;@rpath")
endif()

