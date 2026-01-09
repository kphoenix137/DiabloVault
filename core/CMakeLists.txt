add_library(DiabloVault_core STATIC
    core.cpp
    core.h
)

target_include_directories(DiabloVault_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
