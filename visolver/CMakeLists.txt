cmake_minimum_required(VERSION 3.20)
project(lvi_dhan06 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Eigen (header-only).
#   Windows: vcpkg install eigen3   (and configure with the vcpkg toolchain),
#            or set Eigen3_DIR to an installed Eigen.
#   Debian : sudo apt install libeigen3-dev
find_package(Eigen3 3.4 REQUIRED NO_MODULE)

add_library(lvi_dhan06 STATIC
    src/dhan06.cpp
)
target_include_directories(lvi_dhan06
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(lvi_dhan06 PUBLIC Eigen3::Eigen)

if (MSVC)
    target_compile_options(lvi_dhan06 PRIVATE /W4)
else()
    target_compile_options(lvi_dhan06 PRIVATE -Wall -Wextra)
endif()

add_executable(lvi_demo src/main.cpp)
target_link_libraries(lvi_demo PRIVATE lvi_dhan06)

enable_testing()
add_test(NAME lvi_demo COMMAND lvi_demo)
