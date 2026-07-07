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
    src/PasswordField.h
    src/UiMetrics.h
    src/UiTheme.h
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

# Automatic DLL deployment for Windows. windeployqt drops the Qt runtime next to
# the exe -- the five Qt6 DLLs plus the platforms/ plugin -- so the build folder
# runs stand-alone. The DLL flavor tracks the build type: a Debug build gets the
# 'd'-suffixed debug DLLs + platforms/qwindowsd.dll; Release (and any other type)
# gets the release DLLs + platforms/qwindows.dll.
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        add_custom_command(TARGET fpwdman_qt POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    "$<IF:$<CONFIG:Debug>,--debug,--release>"
                    "$<TARGET_FILE:fpwdman_qt>"
            COMMENT "Deploying Qt libraries to the executable directory..."
        )

        # ---------------------------------------------------------------------
        # 'package' target: a clean, shippable tarball -- NOT built by default.
        #
        # The build folder is full of artifacts a user must not have to sort
        # through (.pdb/.ilk, _deps/, autogen, the test exes, CMake scaffolding).
        # This stages ONLY the runtime into dist/fpwdman_qt/ -- the exe plus the
        # Qt runtime windeployqt certifies as required -- then gzips it into
        # fpwdman_qt.tgz, which unpacks to a single top-level fpwdman_qt/ folder.
        #
        # The stage is wiped and rebuilt every run, so nothing stale survives.
        # Bloat is stripped (translations, software OpenGL, the D3D compiler) and
        # the MSVC runtime is omitted -- recipients are assumed to have the VC++
        # redistributable installed. The DLL flavor tracks the build type, so
        # this is only meaningful against a Release build:
        #     cmake --build <release-build-dir> --target package
        # ---------------------------------------------------------------------
        set(PACKAGE_STAGE_DIR "${CMAKE_BINARY_DIR}/dist/fpwdman_qt")
        add_custom_target(package
            COMMAND "${CMAKE_COMMAND}" -E rm -rf "${PACKAGE_STAGE_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${PACKAGE_STAGE_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "$<TARGET_FILE:fpwdman_qt>" "${PACKAGE_STAGE_DIR}/"
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    "$<IF:$<CONFIG:Debug>,--debug,--release>"
                    --dir "${PACKAGE_STAGE_DIR}"
                    --no-translations
                    --no-opengl-sw
                    --no-system-d3d-compiler
                    --no-compiler-runtime
                    "${PACKAGE_STAGE_DIR}/fpwdman_qt.exe"
            # tar from inside dist/ so the archive holds a single top-level
            # fpwdman_qt/ folder. chdir (not the target's WORKING_DIRECTORY) is
            # used because dist/ does not exist until make_directory above runs;
            # a target-level WORKING_DIRECTORY would cd there before any command.
            COMMAND "${CMAKE_COMMAND}" -E chdir "${CMAKE_BINARY_DIR}/dist"
                    "${CMAKE_COMMAND}" -E tar czf
                    "${CMAKE_BINARY_DIR}/fpwdman_qt.tgz" fpwdman_qt
            COMMENT "Packaging stand-alone fpwdman_qt.tgz (exe + required Qt runtime)"
            VERBATIM
        )
        add_dependencies(package fpwdman_qt)
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
