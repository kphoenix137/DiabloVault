find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(DiabloVault
    main.cpp
)

target_link_libraries(DiabloVault PRIVATE Qt6::Widgets DiabloVault_core)
