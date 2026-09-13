# Copyright Ben Paul Wise. All Rights Reserved.
# hexgames_deploy_qt(<target>): after linking, put the Qt runtime beside the executable.
#
# Windows only; a no-op elsewhere (system Qt).  Without this step a Qt program on Windows starts,
# shows an empty error dialog and exits -- which looks exactly like a thrown exception and is not one
# (see visolver/apps/minppd/plan.md).  The default path runs windeployqt, which chooses the Debug
# (d-suffixed) or Release DLL set by configuration and also brings platforms/, styles/ and any plugin
# the target's Qt modules need.  The fallback reproduces the minimal hand recipe: Core, Gui, Widgets,
# Concurrent and platforms/qwindows.
option(HEXGAMES_USE_WINDEPLOYQT "Deploy Qt with windeployqt (else copy the minimal DLL set)" ON)

function(hexgames_deploy_qt target)
  if(NOT WIN32 OR NOT TARGET Qt6::Core)
    return()
  endif()
  if(HEXGAMES_USE_WINDEPLOYQT AND TARGET Qt6::windeployqt)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND Qt6::windeployqt
              $<$<CONFIG:Debug>:--debug> $<$<NOT:$<CONFIG:Debug>>:--release>
              --no-translations --no-system-d3d-compiler --no-opengl-sw --no-compiler-runtime
              --dir "$<TARGET_FILE_DIR:${target}>" "$<TARGET_FILE:${target}>"
      COMMENT "windeployqt ${target}" VERBATIM)
  else()
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:Qt6::Core> $<TARGET_FILE:Qt6::Gui> $<TARGET_FILE:Qt6::Widgets>
              "$<TARGET_FILE_DIR:${target}>"
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/platforms"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              $<TARGET_FILE:Qt6::QWindowsIntegrationPlugin> "$<TARGET_FILE_DIR:${target}>/platforms"
      COMMENT "copy Qt runtime for ${target}" VERBATIM)
    if(TARGET Qt6::Concurrent)
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Concurrent> "$<TARGET_FILE_DIR:${target}>" VERBATIM)
    endif()
  endif()
endfunction()
# Copyright Ben Paul Wise. All Rights Reserved.
