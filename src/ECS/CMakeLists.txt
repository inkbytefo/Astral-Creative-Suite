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

# Link dependencies
target_link_libraries(AstralECS PUBLIC
    AstralCore
)

# Conditionally link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(AstralECS PUBLIC fmt::fmt)
endif()

target_include_directories(AstralECS PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${PROJECT_SOURCE_DIR}/external/glm
)