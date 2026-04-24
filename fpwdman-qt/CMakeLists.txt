# Copyright Ben Paul Wise. All Rights Reserved.
cmake_minimum_required(VERSION 3.20)
project(fpwdman_qt)

set(CMAKE_CXX_STANDARD 20)

# Recommended Qt 6 setup
set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64")
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(fpwdman_qt
    WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/SiteEntry.h
    src/ViewSiteEntry.h
    src/ViewSiteEntry.cpp
    src/EditSiteEntry.h
    src/EditSiteEntry.cpp
    src/sbc.h
    src/sbc.cpp
)

target_link_libraries(fpwdman_qt PRIVATE Qt6::Widgets)

# Automatic DLL deployment for Windows
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        add_custom_command(TARGET fpwdman_qt POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --debug "$<TARGET_FILE:fpwdman_qt>"
            COMMENT "Deploying Qt libraries to the executable directory..."
        )
    else()
        message(WARNING "windeployqt not found. DLLs will not be deployed automatically.")
    endif()
endif()

# Testing setup
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_executable(sbc_test
    test/SbcTest.cpp
    src/sbc.h
    src/sbc.cpp
)
target_link_libraries(sbc_test PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(sbc_test)

# Copyright Ben Paul Wise. All Rights Reserved.
