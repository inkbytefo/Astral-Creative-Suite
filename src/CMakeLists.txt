# Add subdirectories
add_subdirectory(Core)
add_subdirectory(ECS)
add_subdirectory(Events)
add_subdirectory(Platform)
add_subdirectory(Renderer)
add_subdirectory(UI)
add_subdirectory(2D)
add_subdirectory(3D)
add_subdirectory(Asset)

# Create the main AstralEngine library (empty library that links all sublibraries)
add_library(AstralEngine INTERFACE)

# Link all the sublibraries
target_link_libraries(AstralEngine INTERFACE
    AstralCore
    AstralECS
    AstralEvents
    AstralPlatform
    AstralRenderer
    AstralUI
    Astral2D
    AstralAsset
)

# Add alias for better compatibility
add_library(Astral::Engine ALIAS AstralEngine)

# Include directories
target_include_directories(AstralEngine INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Test executable
add_executable(test_renderer test_renderer.cpp)
target_link_libraries(test_renderer PRIVATE AstralEngine)
