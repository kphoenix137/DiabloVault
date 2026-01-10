find_package(Qt6 CONFIG REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(DiabloVault
  MANUAL_FINALIZATION
  main.cpp
  MainWindow.h MainWindow.cpp
  models/ItemModel.h models/ItemModel.cpp
  models/ContainerModel.h models/ContainerModel.cpp
)

target_link_libraries(DiabloVault PRIVATE Qt6::Core Qt6::Widgets DiabloVault_core)

qt_finalize_executable(DiabloVault)

if (WIN32)
  # Determine vcpkg triplet root: .../vcpkg_installed/x64-windows
  get_target_property(_qt_core_dbg Qt6::Core IMPORTED_LOCATION_DEBUG)
  get_target_property(_qt_core_rel Qt6::Core IMPORTED_LOCATION_RELEASE)

  if (_qt_core_dbg)
    get_filename_component(_qt_bin_dir "${_qt_core_dbg}" DIRECTORY)  # .../debug/bin
  else()
    get_filename_component(_qt_bin_dir "${_qt_core_rel}" DIRECTORY)  # .../bin
  endif()

  get_filename_component(_qt_level1 "${_qt_bin_dir}" DIRECTORY)      # .../debug or .../x64-windows
  get_filename_component(_qt_triplet_root "${_qt_level1}" DIRECTORY) # .../x64-windows

  # Post-build deploy script (robust with vcpkg Qt6 layout: Qt6/plugins and debug/Qt6/plugins)
  set(_deploy_script "${CMAKE_CURRENT_BINARY_DIR}/deploy_qt_plugins.cmake")
  file(WRITE "${_deploy_script}" [=[
# deploy_qt_plugins.cmake
# Required -D args:
#   DST      = directory containing DiabloVault.exe
#   QTROOT   = .../vcpkg_installed/x64-windows
#   IS_DEBUG = 1 or 0

if (NOT DEFINED DST OR NOT DEFINED QTROOT OR NOT DEFINED IS_DEBUG)
  message(FATAL_ERROR "deploy_qt_plugins.cmake: missing required -D args (DST/QTROOT/IS_DEBUG)")
endif()

set(dst "${DST}")
set(root "${QTROOT}")

# vcpkg Qt layout:
#   Release plugins: ${root}/Qt6/plugins
#   Debug plugins:   ${root}/debug/Qt6/plugins
if (IS_DEBUG)
  set(plugroot "${root}/debug/Qt6/plugins")
  set(platform_src "${plugroot}/platforms/qwindowsd.dll")
  set(platform_dst "${dst}/platforms/qwindowsd.dll")
else()
  set(plugroot "${root}/Qt6/plugins")
  set(platform_src "${plugroot}/platforms/qwindows.dll")
  set(platform_dst "${dst}/platforms/qwindows.dll")
endif()

file(MAKE_DIRECTORY "${dst}/platforms")

if (NOT EXISTS "${platform_src}")
  message(FATAL_ERROR "Missing platform plugin: ${platform_src}")
endif()

file(COPY_FILE "${platform_src}" "${platform_dst}" ONLY_IF_DIFFERENT)

# Optional plugin folders (copy only if present)
foreach(subdir imageformats iconengines platformthemes styles)
  if (EXISTS "${plugroot}/${subdir}")
    file(MAKE_DIRECTORY "${dst}/${subdir}")
    file(COPY "${plugroot}/${subdir}/" DESTINATION "${dst}/${subdir}")
  endif()
endforeach()
]=])

  add_custom_command(TARGET DiabloVault POST_BUILD
    COMMAND ${CMAKE_COMMAND}
      -DDST=$<TARGET_FILE_DIR:DiabloVault>
      -DQTROOT=${_qt_triplet_root}
      -DIS_DEBUG=$<$<CONFIG:Debug>:1>$<$<NOT:$<CONFIG:Debug>>:0>
      -P "${_deploy_script}"
    VERBATIM
  )
endif()
