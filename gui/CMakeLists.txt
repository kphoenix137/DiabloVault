find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(DiabloVault
    main.cpp
    MainWindow.h MainWindow.cpp
    models/ItemModel.h models/ItemModel.cpp
    models/ContainerModel.h models/ContainerModel.cpp
)

target_link_libraries(DiabloVault PRIVATE Qt6::Widgets DiabloVault_core)

qt_finalize_executable(DiabloVault)

if (WIN32)
  add_custom_command(TARGET DiabloVault POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory
      "$<TARGET_FILE_DIR:DiabloVault>/platforms"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_BINARY_DIR}/vcpkg_installed/$<IF:$<CONFIG:Debug>,x64-windows/debug,x64-windows>/Qt6/plugins/platforms/qwindows$<$<CONFIG:Debug>:d>.dll"
      "$<TARGET_FILE_DIR:DiabloVault>/platforms/"
  )
endif()
