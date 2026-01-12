add_library(DiabloVault_core STATIC
    Workspace.cpp
    Workspace.h

    devx/codec.cpp
    devx/codec.h
    devx/sha.cpp
    devx/sha.h

    d1/MpqStorm.h
    d1/MpqStorm.cpp
)

target_include_directories(DiabloVault_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

 # DiabloVault always supports reading packed MPQ saves.
 target_compile_definitions(DiabloVault_core PUBLIC DIABLOVAULT_ENABLE_STORMLIB=1)

# These must exist because StormLib::storm's INTERFACE_LINK_LIBRARIES references them.
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)

# StormLib is provided by vcpkg as a config package.
find_package(StormLib CONFIG REQUIRED)

if(TARGET StormLib::StormLib)
    target_link_libraries(DiabloVault_core PRIVATE StormLib::StormLib)
elseif(TARGET StormLib::storm)
    target_link_libraries(DiabloVault_core PRIVATE StormLib::storm)
else()
    message(FATAL_ERROR "StormLib package found but no known target exported (expected StormLib::StormLib or StormLib::storm)")
endif()
