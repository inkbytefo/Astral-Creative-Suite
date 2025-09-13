# 3D library
set(D3_SOURCES
    Render3DSystem.cpp
)

set(D3_HEADERS
    Components3D.h
    Render3DSystem.h
)

# Create the Astral3D library
add_library(Astral3D ${D3_SOURCES} ${D3_HEADERS})

# Define alias for better compatibility
add_library(Astral::D3 ALIAS Astral3D)

# Link dependencies
target_link_libraries(Astral3D PUBLIC
    AstralCore
    AstralECS
    AstralRenderer
)

# Include directories
target_include_directories(Astral3D PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compiler definitions
target_compile_definitions(Astral3D PUBLIC GLM_ENABLE_EXPERIMENTAL)