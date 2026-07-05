# Copyright Ben Paul Wise. All Rights Reserved.
cmake_minimum_required(VERSION 3.20)
project(fpwdman_qt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recommended Qt 6 setup
set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64")
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

include(FetchContent)

# -----------------------------------------------------------------------------
# zlib: needed ONLY by the legacy .sbc reader (raw zlib inflate). Fetched and
# built statically so the project stays self-contained (no find_package(ZLIB),
# which is unreliable under MSVC). The modern container does not use zlib.
# -----------------------------------------------------------------------------
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  zlib
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG v1.3.1
)
FetchContent_MakeAvailable(zlib)

# -----------------------------------------------------------------------------
# sbc: the in-project Sliding Block Cipher library. Toolkit-agnostic (no Qt).
#   sbc_core   -- the cipher engine (shared by legacy + modern paths)
#   sbc_legacy -- read-only decoder for old .sbc files (uses zlib)
# -----------------------------------------------------------------------------
add_library(sbc STATIC
    src/sbc/sbc_core.cpp
    src/sbc/sbc_legacy.cpp
)
target_include_directories(sbc PUBLIC src/sbc)
target_include_directories(sbc PRIVATE ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
target_link_libraries(sbc PRIVATE zlibstatic)

# -----------------------------------------------------------------------------
# pwcrypto: Qt-side crypto -- the modern container (PasswordStore) and the
# password generator. Depends on QtCore + the sbc cipher. Shared by the app and
# the store tests. No Q_OBJECT here, so no moc is needed.
# -----------------------------------------------------------------------------
add_library(pwcrypto STATIC
    src/crypto/PasswordStore.cpp
    src/crypto/PasswordGenerator.cpp
)
target_include_directories(pwcrypto PUBLIC src/crypto src)
target_link_libraries(pwcrypto PUBLIC Qt6::Core sbc)

# -----------------------------------------------------------------------------
# The Qt application
# -----------------------------------------------------------------------------
qt_add_executable(fpwdman_qt
    WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/SiteEntry.h
    src/SiteDatabase.h
    src/ClipboardUtil.h
    src/UiMetrics.h
    src/ViewSiteEntry.h
    src/ViewSiteEntry.cpp
    src/EditSiteEntry.h
    src/EditSiteEntry.cpp
    src/EnterMasterPassphraseDialog.h
    src/EnterMasterPassphraseDialog.cpp
    src/ChangeMasterPassphraseDialog.h
    src/ChangeMasterPassphraseDialog.cpp
    src/PreferencesDialog.h
    src/PreferencesDialog.cpp
)

target_link_libraries(fpwdman_qt PRIVATE Qt6::Widgets sbc pwcrypto)

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

# -----------------------------------------------------------------------------
# Testing
# -----------------------------------------------------------------------------
enable_testing()

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# sbc_test: Qt-free unit tests for the cipher core and the legacy reader,
# including the golden-vector decrypt of a real old .sbc file.
add_executable(sbc_test
    test/SbcCoreTest.cpp
    test/SbcLegacyTest.cpp
)
target_link_libraries(sbc_test PRIVATE sbc GTest::gtest GTest::gtest_main)
target_compile_definitions(sbc_test PRIVATE
    SBC_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/data")
gtest_discover_tests(sbc_test)

# store_test: Qt-linked tests for the modern container, PBKDF2, XML mapping,
# and the password generator. DISCOVERY_MODE PRE_TEST defers running the exe
# (which needs the Qt Core DLL) from build time to test time.
add_executable(store_test
    test/StoreTest.cpp
)
target_link_libraries(store_test PRIVATE pwcrypto GTest::gtest GTest::gtest_main)
gtest_discover_tests(store_test DISCOVERY_MODE PRE_TEST)

# Copyright Ben Paul Wise. All Rights Reserved.
