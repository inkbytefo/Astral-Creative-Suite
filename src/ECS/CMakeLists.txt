# ECS library
set(ECS_SOURCES
    ArchetypeECS.cpp
    Components.cpp
    Scene.cpp
)

set(ECS_HEADERS
    ArchetypeECS.h
    Components.h
    ECS.h
    Entity.h
    RenderComponents.h
    Scene.h
)

add_library(AstralECS ${ECS_SOURCES} ${ECS_HEADERS})

# Define alias for better compatibility
add_library(Astral::ECS ALIAS AstralECS)

# Link dependencies
target_link_libraries(AstralECS PUBLIC
    AstralCore
)

target_include_directories(AstralECS PUBLIC
    ${PROJECT_SOURCE_DIR}/external/vma/include
)

target_compile_definitions(AstralECS PUBLIC GLM_ENABLE_EXPERIMENTAL)

# Conditionally link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(AstralECS PUBLIC fmt::fmt)
endif()

target_include_directories(AstralECS PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${PROJECT_SOURCE_DIR}/external/glm
)
