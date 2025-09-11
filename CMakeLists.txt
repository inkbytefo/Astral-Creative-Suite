cmake_minimum_required(VERSION 3.20)
project(AstralEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Vulkan SDK'nın konumunu belirt (ortam değişkeni veya sabit yol)
set(VULKAN_SDK $ENV{VULKAN_SDK})
if(NOT VULKAN_SDK)
    message(FATAL_ERROR "VULKAN_SDK ortam değişkeni bulunamadı! Lütfen Vulkan SDK'yı yükleyin ve ortam değişkenini ayarlayın.")
endif()

# Vulkan SDK header ve library yollarını ekle
include_directories(${VULKAN_SDK}/Include)
link_directories(${VULKAN_SDK}/Lib)

# --- Vulkan SDK'nın CMake desteğini bul ---
# Bu, Vulkan::Vulkan hedefini tanımlar
find_package(Vulkan REQUIRED)

# --- SDL3 Kütüphanesini Tanımla ---
# SDL3'ü external klasöründen kullanmak için CMake yapılandırması
set(SDL3_DIR ${PROJECT_SOURCE_DIR}/external/SDL3/cmake)
if(EXISTS ${SDL3_DIR})
    find_package(SDL3 REQUIRED CONFIG PATHS ${SDL3_DIR})
else()
    # Fallback to system SDL3 if external is not available
    find_package(SDL3 REQUIRED)
endif()

# --- fmt Kütüphanesini Tanımla ---
# Check if fmt exists in external, otherwise use system
if(EXISTS ${PROJECT_SOURCE_DIR}/external/fmt AND IS_DIRECTORY ${PROJECT_SOURCE_DIR}/external/fmt)
    add_subdirectory(external/fmt)
    set(FMT_FOUND TRUE)
else()
    # Try to find system fmt
    find_package(fmt QUIET)
    if(fmt_FOUND)
        set(FMT_FOUND TRUE)
    else()
        set(FMT_FOUND FALSE)
    endif()
endif()

# --- Harici Kütüphaneleri Tanımla ---
# Check if tinyobjloader exists in external, otherwise it might be system
if(EXISTS ${PROJECT_SOURCE_DIR}/external/tinyobjloader AND IS_DIRECTORY ${PROJECT_SOURCE_DIR}/external/tinyobjloader)
    add_subdirectory(external/tinyobjloader)
endif()

# shaderc removed - using offline glslc compilation only

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${PROJECT_SOURCE_DIR}/external/stb)

# --- Kaynak Kodunu ve Kütüphaneyi Tanımla ---
add_subdirectory(src)

# --- Kütüphanenin Bağımlılıklarını Belirt ---
# PUBLIC: Bu kütüphaneyi kullanan diğer hedeflerin de bu yolları ve kütüphaneleri görmesini sağlar.
# Bu tek komut, hem AstralEngine'in hem de AstralEditor'ın ihtiyaç duyduğu her şeyi bağlar.
# Note: AstralEngine is now an INTERFACE library, so we link its dependencies directly to targets that need them

# --- Çalıştırılabilir Dosyaları Tanımla ---
# New 2D graphics editor executable
add_executable(AstralCreativeSuite src/editor_main.cpp)

# --- Çalıştırılabilir Dosyaların Bağımlılıklarını Belirt ---
target_link_libraries(AstralCreativeSuite PRIVATE 
    AstralECS
    AstralEvents
    AstralPlatform
    AstralRenderer
    AstralUI
    Astral2D
    AstralAsset
    Vulkan::Vulkan
    SDL3::SDL3
    tinyobjloader
    stb_image
)

# Add fmt if found
if(FMT_FOUND)
    target_link_libraries(AstralCreativeSuite PRIVATE fmt::fmt)
endif()

# GLM'in deneysel özelliklerini kullanmak için bu tanımı ekleyin
target_compile_definitions(AstralCreativeSuite PRIVATE GLM_ENABLE_EXPERIMENTAL)

target_include_directories(AstralCreativeSuite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external/glm
    ${PROJECT_SOURCE_DIR}/external/tinyobjloader
    ${PROJECT_SOURCE_DIR}/external/vma/include # VMA için 'include' alt klasörünü belirtiyoruz
)

# --- Platforma Bağımlı Derleme Sonrası Adımlar ---
if(WIN32)
    # Windows için DLL'leri kopyala
    # Copy DLLs for the new editor as well
    if(TARGET SDL3::SDL3)
        add_custom_command(
            TARGET AstralCreativeSuite POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:AstralCreativeSuite>
            COMMENT "SDL3.dll kopyalanıyor (AstralCreativeSuite)..."
        )
    endif()
endif()

# Copy assets for the new editor as well
add_custom_command(
    TARGET AstralCreativeSuite POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:AstralCreativeSuite>/assets"
    COMMENT "Assets kopyalanıyor (AstralCreativeSuite)..."
)

# Shader compilation section - only compile 2D shaders for the creative suite
set(SHADERS_SRC_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SPV_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders_spv")

# Ensure output directory exists
file(MAKE_DIRECTORY ${SPV_OUTPUT_DIR})

# Find glslc (ShaderC compiler)
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS ${VULKAN_SDK}/bin)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please install Vulkan SDK.")
endif()

# 2D Canvas shaders
add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/2d_canvas.vert.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/2d_canvas.vert" -o "${SPV_OUTPUT_DIR}/2d_canvas.vert.spv"
    DEPENDS "${SHADERS_SRC_DIR}/2d_canvas.vert"
    COMMENT "glslc 2d_canvas.vert -> 2d_canvas.vert.spv"
)

add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/2d_canvas.frag.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/2d_canvas.frag" -o "${SPV_OUTPUT_DIR}/2d_canvas.frag.spv"
    DEPENDS "${SHADERS_SRC_DIR}/2d_canvas.frag"
    COMMENT "glslc 2d_canvas.frag -> 2d_canvas.frag.spv"
)

# 2D Brush shaders
add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/2d_brush.vert.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/2d_brush.vert" -o "${SPV_OUTPUT_DIR}/2d_brush.vert.spv"
    DEPENDS "${SHADERS_SRC_DIR}/2d_brush.vert"
    COMMENT "glslc 2d_brush.vert -> 2d_brush.vert.spv"
)

add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/2d_brush.frag.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/2d_brush.frag" -o "${SPV_OUTPUT_DIR}/2d_brush.frag.spv"
    DEPENDS "${SHADERS_SRC_DIR}/2d_brush.frag"
    COMMENT "glslc 2d_brush.frag -> 2d_brush.frag.spv"
)

# 2D Grid shaders
add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/2d_grid.vert.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/2d_grid.vert" -o "${SPV_OUTPUT_DIR}/2d_grid.vert.spv"
    DEPENDS "${SHADERS_SRC_DIR}/2d_grid.vert"
    COMMENT "glslc 2d_grid.vert -> 2d_grid.vert.spv"
)

add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/2d_grid.frag.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/2d_grid.frag" -o "${SPV_OUTPUT_DIR}/2d_grid.frag.spv"
    DEPENDS "${SHADERS_SRC_DIR}/2d_grid.frag"
    COMMENT "glslc 2d_grid.frag -> 2d_grid.frag.spv"
)

# 3D Shaders (for backward compatibility)
add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/unified_pbr.vert.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/unified_pbr.vert" -o "${SPV_OUTPUT_DIR}/unified_pbr.vert.spv"
    DEPENDS "${SHADERS_SRC_DIR}/unified_pbr.vert"
    COMMENT "glslc unified_pbr.vert -> unified_pbr.vert.spv"
)

add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/unified_pbr.frag.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/unified_pbr.frag" -o "${SPV_OUTPUT_DIR}/unified_pbr.frag.spv"
    DEPENDS "${SHADERS_SRC_DIR}/unified_pbr.frag"
    COMMENT "glslc unified_pbr.frag -> unified_pbr.frag.spv"
)

add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/basic.vert.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/basic.vert" -o "${SPV_OUTPUT_DIR}/basic.vert.spv"
    DEPENDS "${SHADERS_SRC_DIR}/basic.vert"
    COMMENT "glslc basic.vert -> basic.vert.spv"
)

add_custom_command(
    OUTPUT "${SPV_OUTPUT_DIR}/basic.frag.spv"
    COMMAND ${GLSLC_EXECUTABLE} "${SHADERS_SRC_DIR}/basic.frag" -o "${SPV_OUTPUT_DIR}/basic.frag.spv"
    DEPENDS "${SHADERS_SRC_DIR}/basic.frag"
    COMMENT "glslc basic.frag -> basic.frag.spv"
)

# Collect all shader outputs
set(SPV_OUTPUTS
    "${SPV_OUTPUT_DIR}/2d_canvas.vert.spv"
    "${SPV_OUTPUT_DIR}/2d_canvas.frag.spv"
    "${SPV_OUTPUT_DIR}/2d_brush.vert.spv"
    "${SPV_OUTPUT_DIR}/2d_brush.frag.spv"
    "${SPV_OUTPUT_DIR}/2d_grid.vert.spv"
    "${SPV_OUTPUT_DIR}/2d_grid.frag.spv"
    "${SPV_OUTPUT_DIR}/unified_pbr.vert.spv"
    "${SPV_OUTPUT_DIR}/unified_pbr.frag.spv"
    "${SPV_OUTPUT_DIR}/basic.vert.spv"
    "${SPV_OUTPUT_DIR}/basic.frag.spv"
)

# Custom target to build all shaders
add_custom_target(shaders ALL DEPENDS ${SPV_OUTPUTS})

# Copy shaders for the new editor as well
add_custom_command(
    TARGET AstralCreativeSuite POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AstralCreativeSuite>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SPV_OUTPUT_DIR}" "$<TARGET_FILE_DIR:AstralCreativeSuite>/shaders"
    COMMENT "Compiled SPIR-V shaderlar kopyalanıyor (AstralCreativeSuite)..."
)

# Copy source shaders for the new editor as well
add_custom_command(
    TARGET AstralCreativeSuite POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:AstralCreativeSuite>/shaders_src"
    COMMENT "GLSL kaynak shaderlar (opsiyonel) kopyalanıyor (AstralCreativeSuite)..."
)

# Pipeline cache dosyasını çıktı dizinine kopyala (sadece varsa)
if(EXISTS "${PROJECT_SOURCE_DIR}/pipeline_cache.bin")
    # Copy pipeline cache for the new editor as well
    add_custom_command(
        TARGET AstralCreativeSuite POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/pipeline_cache.bin" "$<TARGET_FILE_DIR:AstralCreativeSuite>/pipeline_cache.bin"
        COMMENT "Pipeline cache kopyalanıyor (AstralCreativeSuite)..."
    )
endif()

# Install targets for clean output directory structure
install(TARGETS AstralCreativeSuite
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install required files to output directory
install(DIRECTORY ${SPV_OUTPUT_DIR}/
    DESTINATION bin/shaders
    FILES_MATCHING PATTERN "*.spv"
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/
    DESTINATION bin/assets
)

# Install SDL3.dll if it exists
if(TARGET SDL3::SDL3)
    install(FILES $<TARGET_FILE:SDL3::SDL3>
        DESTINATION bin
    )
endif()