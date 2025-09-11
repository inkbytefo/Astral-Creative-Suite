# Core library
set(CORE_SOURCES
    Logger.cpp
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

# Link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(AstralCore PUBLIC fmt::fmt)
endif()

target_include_directories(AstralCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)