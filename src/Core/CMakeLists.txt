# Core library
set(CORE_SOURCES
    Logger.cpp
    MemoryManager.cpp
    AssetManager.cpp
    PerformanceMonitor.cpp
    AssetLocator.cpp
    AssetDependency.cpp
)

set(CORE_HEADERS
    Logger.h
    EngineConfig.h
    MemoryManager.h
    AssetLocator.h
    AssetManager.h
    PerformanceMonitor.h
    AssetDependency.h
)

add_library(AstralCore ${CORE_SOURCES} ${CORE_HEADERS})

# Define alias for better compatibility
add_library(Astral::Core ALIAS AstralCore)

# Link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(AstralCore PUBLIC
    fmt::fmt
)
endif()

target_include_directories(AstralCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)
