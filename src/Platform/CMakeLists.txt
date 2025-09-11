# Platform library
set(PLATFORM_SOURCES
    Window.cpp
)

set(PLATFORM_HEADERS
    Window.h
)

add_library(AstralPlatform ${PLATFORM_SOURCES} ${PLATFORM_HEADERS})

# Link dependencies
target_link_libraries(AstralPlatform PUBLIC
    SDL3::SDL3
    AstralCore
)

# Conditionally link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(AstralPlatform PUBLIC fmt::fmt)
endif()

target_include_directories(AstralPlatform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${PROJECT_SOURCE_DIR}/external/glm
)