# Asset library
set(ASSET_SOURCES
    ModelAsset.cpp
    MeshAsset.cpp
    MaterialAsset.cpp
    ModelLoader.cpp
)

set(ASSET_HEADERS
    AssetLocator.h
    ImageAssetManager.h
    ModelAsset.h
    MeshAsset.h
    MaterialAsset.h
    ModelLoader.h
)

add_library(AstralAsset ${ASSET_SOURCES} ${ASSET_HEADERS})

# Link dependencies
target_link_libraries(AstralAsset PUBLIC
    AstralCore
    stb_image
    tinyobjloader
)

# Conditionally link fmt if available
if(TARGET fmt::fmt)
    target_link_libraries(AstralAsset PUBLIC fmt::fmt)
endif()

target_include_directories(AstralAsset PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)
