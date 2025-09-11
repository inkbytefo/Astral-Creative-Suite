# UI library
set(UI_SOURCES
    UIManager.cpp
)

set(UI_HEADERS
    UIManager.h
)

add_library(AstralUI ${UI_SOURCES} ${UI_HEADERS})

# Link dependencies
target_link_libraries(AstralUI PUBLIC
    AstralCore
)

# Conditionally link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(AstralUI PUBLIC fmt::fmt)
endif()

target_include_directories(AstralUI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${PROJECT_SOURCE_DIR}/external/glm
)