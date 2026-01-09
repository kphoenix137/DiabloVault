add_library(DiabloVault_core STATIC
    Workspace.cpp
    Workspace.h
)

target_include_directories(DiabloVault_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
