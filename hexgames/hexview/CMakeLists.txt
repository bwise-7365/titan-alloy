# Copyright Ben Paul Wise. All Rights Reserved.
# hexview -- the presentation model, no Qt (M10). Only the [PROPOSED] contract headers exist so far.
# hexview_contracts compiles every header once, so a contract that does not compile is found before
# anyone implements it. It is EXCLUDE_FROM_ALL: ordinary builds, and the workers' builds, skip it.
# Build it on request:
#   cmake --build --preset win-msvc-debug --target hexview_contracts
# When the implementation lands, a STATIC hexview library replaces this target.
add_library(hexview_contracts OBJECT EXCLUDE_FROM_ALL ContractsCompile.cpp)
target_include_directories(hexview_contracts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(hexview_contracts PUBLIC hexrecord hexengine hexrules hexxml hexmodel hexcoord)
target_compile_features(hexview_contracts PUBLIC cxx_std_20)
# Copyright Ben Paul Wise. All Rights Reserved.
