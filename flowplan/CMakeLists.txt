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

# Shared library exposing the planner to Java via JNI. The FFI path now calls
# runGLPK(), so fplp.cpp and the glpk::glpk usage requirements (include dir +
# static lib) are linked in below.
find_package(JNI REQUIRED)
add_library(flowplan_jni SHARED
        jni/flowplan_jni.cpp
        lib/flowplan_ffi.cpp
        lib/flowplanner.cpp
        lib/fplp.cpp   # have to include GLPK
        lib/utils.cpp            # balancedSD symbol referenced by initGM
)
target_include_directories(flowplan_jni PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        ${JNI_INCLUDE_DIRS}
)
target_link_libraries(flowplan_jni PRIVATE ${JNI_LIBRARIES} glpk::glpk)

# --- Java JNI test driver -------------------------------------------------
# CLion is a CMake/C++ IDE with no Java module, so FPJNITest.java shows a main()
# but no run arrow. Expose it as a custom target instead: a target CLion CAN run
# from the IDE (and the command line). It compiles FlowPlannerNative + FPJNITest
# into the build dir, then runs FPJNITest with the freshly built native lib on
# java.library.path. DEPENDS on flowplan_jni so the DLL is up to date first.
#
# Build/run it with:  cmake --build <build-dir> --target flowplan_jni_test
# or pick "flowplan_jni_test" from CLion's run/target dropdown.
set(_jdk_bin "${JAVA_INCLUDE_PATH}/../bin")
find_program(Java_JAVAC_EXECUTABLE NAMES javac HINTS "${_jdk_bin}" "$ENV{JAVA_HOME}/bin")
find_program(Java_JAVA_EXECUTABLE  NAMES java  HINTS "${_jdk_bin}" "$ENV{JAVA_HOME}/bin")

add_custom_target(flowplan_jni_test
        COMMAND ${Java_JAVAC_EXECUTABLE} -d ${CMAKE_CURRENT_BINARY_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/jni/FlowPlannerNative.java
                ${CMAKE_CURRENT_SOURCE_DIR}/jni/FPJNITest.java
        COMMAND ${Java_JAVA_EXECUTABLE}
                "-Djava.library.path=$<TARGET_FILE_DIR:flowplan_jni>"
                -cp ${CMAKE_CURRENT_BINARY_DIR}
                FPJNITest
        DEPENDS flowplan_jni
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Compiling and running the FPJNITest JNI driver"
        USES_TERMINAL
        VERBATIM
)
