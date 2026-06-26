cmake_minimum_required(VERSION 3.20)
project(flowplan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Per-configuration compile/link flags (MSVC).
# Applies to every target defined after this point.
add_compile_options(
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Release>:/O2>
)
add_link_options(
        $<$<CONFIG:Debug>:/DEBUG>
)

# Locate the installed GLPK package. The line below is just a search hint;
# it can be removed if CMAKE_PREFIX_PATH is supplied externally (-DCMAKE_PREFIX_PATH=C:/glpk).
list(APPEND CMAKE_PREFIX_PATH "C:/glpk")
find_package(glpk CONFIG REQUIRED)

add_executable(flowplan

        src/main.cpp

        lib/flowplanner.h
        lib/flowplanner.cpp
        lib/utils.cpp
        lib/fplp.cpp
)

add_executable( simple
        src/simple.cpp

)

target_include_directories(flowplan PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# glpk::glpk carries the include dir (C:/glpk/include) and the static lib automatically
target_link_libraries(flowplan PRIVATE glpk::glpk)
target_link_libraries(simple   PRIVATE glpk::glpk)
