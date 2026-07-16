# Copyright Ben Paul Wise. All Rights Reserved.
cmake_minimum_required(VERSION 3.20)
project(fpwdman_qt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Link flavor.
#
# OFF (the default) is the stock build: the exe is linked against the Qt DLLs in
# C:/Qt and ships beside a directory of them.
#
# ON links a Qt that was compiled from source as static libraries, plus the
# static MSVC runtime, giving a single .exe that runs on a bare Windows box --
# no Qt DLLs, no platforms/ folder, no VC++ redistributable.
#
# The static Qt is NOT the one under C:/Qt: that one is DLLs only and cannot be
# linked statically. It has to be built first, once. doc/static-build.md has the
# recipe. Then:
#     cmake -B cmake-build-static -G Ninja -DCMAKE_BUILD_TYPE=Release -DFPWDMAN_STATIC=ON
#     cmake --build cmake-build-static
# -----------------------------------------------------------------------------
option(FPWDMAN_STATIC "Link Qt and the CRT statically: one self-contained .exe" OFF)

set(FPWDMAN_QT_SHARED_DIR "C:/Qt/6.8.3/msvc2022_64"
    CACHE PATH "Prefix of the stock DLL Qt, used by the default build")
set(FPWDMAN_QT_STATIC_DIR "C:/Qt/6.8.3-static"
    CACHE PATH "Prefix of the self-built static Qt, used when FPWDMAN_STATIC is ON")

if(FPWDMAN_STATIC)
    if(NOT EXISTS "${FPWDMAN_QT_STATIC_DIR}")
        message(FATAL_ERROR
            "FPWDMAN_STATIC is ON but there is no static Qt at ${FPWDMAN_QT_STATIC_DIR}. "
            "Build one first -- see doc/static-build.md -- or point "
            "-DFPWDMAN_QT_STATIC_DIR at an existing one.")
    endif()

    # doc/static-build.md builds Release only, so a static Qt has no debug
    # (/MTd) libraries. A Debug configure would sail past find_package and then
    # die deep in the link on unresolved CRT symbols, which reads as nonsense.
    # Refuse up front instead.
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(FATAL_ERROR
            "FPWDMAN_STATIC is a Release-only configuration: the static Qt at "
            "${FPWDMAN_QT_STATIC_DIR} is built -release -static-runtime and ships no "
            "debug libraries. Configure with -DCMAKE_BUILD_TYPE=Release, or drop "
            "FPWDMAN_STATIC and debug against the DLL Qt.")
    endif()

    set(CMAKE_PREFIX_PATH "${FPWDMAN_QT_STATIC_DIR}")

    # The static Qt is configured -static-runtime, so its .lib files carry the
    # static CRT (/MT). Every object in this build must agree: mixing /MD and /MT
    # fails the link on duplicate and unresolved CRT symbols. This one variable
    # covers our own targets and the FetchContent dependencies (zlib, gtest)
    # alike -- but only under policy CMP0091, which is what moves CRT selection
    # out of CMAKE_{C,CXX}_FLAGS and into this abstraction. Our own
    # cmake_minimum_required(3.20) already implies CMP0091 NEW; the
    # CMAKE_POLICY_DEFAULT line forces it on the fetched subprojects too, whose
    # older cmake_minimum_required would otherwise reset the policy to OLD and
    # bake a literal /MD back into their flags.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
else()
    set(CMAKE_PREFIX_PATH "${FPWDMAN_QT_SHARED_DIR}")
endif()

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
#   sbc_kdf    -- SHA-256 / HMAC / PBKDF2 for the modern container's key
#                 derivation. Lives here, not beside PasswordStore, because Qt's
#                 hash classes are non-copyable and so cannot express a fast
#                 PBKDF2 -- see doc/kdf-performance-and-security.md.
# -----------------------------------------------------------------------------
add_library(sbc STATIC
    src/sbc/sbc_core.cpp
    src/sbc/sbc_kdf.cpp
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

if(FPWDMAN_STATIC)
    # Nothing to deploy: the exe is the entire product, so there is no
    # windeployqt step and the build folder runs stand-alone by construction.
    # 'package' still exists, and still produces fpwdman_qt.tgz unpacking to a
    # single fpwdman_qt/ folder, so the shipping command does not change with the
    # link flavor -- there is just one file to stage instead of a runtime tree.
    set(PACKAGE_STAGE_DIR "${CMAKE_BINARY_DIR}/dist/fpwdman_qt")
    add_custom_target(package
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${PACKAGE_STAGE_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${PACKAGE_STAGE_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:fpwdman_qt>" "${PACKAGE_STAGE_DIR}/"
        COMMAND "${CMAKE_COMMAND}" -E chdir "${CMAKE_BINARY_DIR}/dist"
                "${CMAKE_COMMAND}" -E tar czf
                "${CMAKE_BINARY_DIR}/fpwdman_qt.tgz" fpwdman_qt
        COMMENT "Packaging stand-alone fpwdman_qt.tgz (single static exe)"
        VERBATIM
    )
    add_dependencies(package fpwdman_qt)

# Automatic DLL deployment for Windows. windeployqt drops the Qt runtime next to
# the exe -- the five Qt6 DLLs plus the platforms/ plugin -- so the build folder
# runs stand-alone. The DLL flavor tracks the build type: a Debug build gets the
# 'd'-suffixed debug DLLs + platforms/qwindowsd.dll; Release (and any other type)
# gets the release DLLs + platforms/qwindows.dll.
elseif(WIN32)
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
# For Windows: keep gtest on the same CRT as the rest of the build, rather than
# letting it override our compiler/linker settings. ON is gtest's own name for
# the shared CRT (/MD), which is right for the default DLL build; under
# FPWDMAN_STATIC everything is /MT and gtest has to follow, or the test exes
# fail to link.
if(FPWDMAN_STATIC)
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
else()
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# sbc_test: Qt-free unit tests for the cipher core, the legacy reader and the
# KDF primitives -- including the golden-vector decrypt of a real old .sbc file
# and the published SHA-256/HMAC/PBKDF2 vectors the file format depends on.
add_executable(sbc_test
    test/SbcCoreTest.cpp
    test/SbcKdfTest.cpp
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
