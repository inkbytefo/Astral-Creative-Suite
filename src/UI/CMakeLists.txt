# UI library
set(UI_SOURCES
    UIManager.cpp
)

set(UI_HEADERS
    UIManager.h
)

add_library(AstralUI ${UI_SOURCES} ${UI_HEADERS})

# Define alias for better compatibility
add_library(Astral::UI ALIAS AstralUI)

# Link dependencies
target_link_libraries(AstralUI PUBLIC
    AstralCore
    AstralPlatform
    AstralRenderer
    AstralECS
    imgui
)

target_include_directories(AstralUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${PROJECT_SOURCE_DIR}/external/glm
)
