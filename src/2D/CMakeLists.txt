# 2D Graphics library
set(2D_SOURCES
    Canvas/Canvas.cpp
    Layers/Layer.cpp
    Tools/Brush.cpp
    Tools/Tool.cpp
)

set(2D_HEADERS
    Canvas/Canvas.h
    Layers/Layer.h
    Tools/Brush.h
    Tools/Tool.h
)

add_library(Astral2D ${2D_SOURCES} ${2D_HEADERS})

# Link dependencies
target_link_libraries(Astral2D PUBLIC
    AstralCore
    AstralEvents
    AstralECS
    stb_image
)

# Conditionally link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(Astral2D PUBLIC fmt::fmt)
endif()

target_include_directories(Astral2D PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    ${PROJECT_SOURCE_DIR}/external/glm
)
