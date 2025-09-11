# Events library
set(EVENTS_SOURCES
    EventManager.cpp
)

set(EVENTS_HEADERS
    ApplicationEvent.h
    Event.h
    EventManager.h
    Events.h
    InputEvent.h
)

add_library(AstralEvents ${EVENTS_SOURCES} ${EVENTS_HEADERS})

# Link dependencies
target_link_libraries(AstralEvents PUBLIC
    AstralCore
)

# Conditionally link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(AstralEvents PUBLIC fmt::fmt)
endif()

target_include_directories(AstralEvents PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)