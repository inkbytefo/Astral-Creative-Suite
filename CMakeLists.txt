cmake_minimum_required(VERSION 3.20)
project(AstralCreativeSuite VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# Find Vulkan
find_package(Vulkan REQUIRED COMPONENTS glslc glslangValidator)

# Vulkan SDK paths
set(VULKAN_SDK $ENV{VULKAN_SDK})
if(VULKAN_SDK)
    message(STATUS "Vulkan SDK found at: ${VULKAN_SDK}")
    include_directories(${VULKAN_SDK}/Include)
    link_directories(${VULKAN_SDK}/Lib)
endif()

# --- SDL3 Kütüphanesini Tanımla ---
set(SDL3_DIR ${PROJECT_SOURCE_DIR}/external/SDL3/cmake)
if(EXISTS ${SDL3_DIR})
    find_package(SDL3 REQUIRED CONFIG PATHS ${SDL3_DIR})
else()
    find_package(SDL3 REQUIRED)
endif()

include(FetchContent)

# --- fmt Kütüphanesini Tanımla (FetchContent) ---
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.2.1 # Or a specific commit/tag you prefer
)
FetchContent_MakeAvailable(fmt)

# --- ImGui Kütüphanesini Manuel Olarak Oluştur ---
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.89.9
)
FetchContent_MakeAvailable(imgui)

# ImGui library oluştur
add_library(imgui STATIC)
target_sources(imgui PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)

target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${VULKAN_SDK}/Include
)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_LOG_FATAL_ERROR)

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

# New 3D editor executable
add_executable(Astral3DEditor src/3d_editor_main.cpp)

# --- Çalıştırılabilir Dosyaların Bağımlılıklarını Belirt ---
target_link_libraries(AstralCreativeSuite PRIVATE 
    AstralEngine
    Vulkan::Vulkan
    SDL3::SDL3
    fmt::fmt
    imgui
)

target_link_libraries(Astral3DEditor PRIVATE 
    AstralEngine
    Vulkan::Vulkan
    SDL3::SDL3
    fmt::fmt
    imgui
)

# GLM'in deneysel özelliklerini kullanmak için bu tanımı ekleyin
target_compile_definitions(AstralCreativeSuite PRIVATE GLM_ENABLE_EXPERIMENTAL)

target_include_directories(AstralCreativeSuite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external/glm
    ${PROJECT_SOURCE_DIR}/external/tinyobjloader
    ${PROJECT_SOURCE_DIR}/external/vma/include # VMA için 'include' alt klasörünü belirtiyoruz
)

target_include_directories(Astral3DEditor PRIVATE
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

# ==============================================================================
# SHADER COMPILATION (AUTOMATED)
# ==============================================================================
set(SHADERS_SRC_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SPV_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders_spv")
file(MAKE_DIRECTORY ${SPV_OUTPUT_DIR})

# Find glslc (ShaderC compiler)
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS ${VULKAN_SDK}/bin)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please install Vulkan SDK and ensure it's in your PATH.")
endif()

# Find all vertex and fragment shaders
file(GLOB SHADER_SOURCES 
    "${SHADERS_SRC_DIR}/*.vert"
    "${SHADERS_SRC_DIR}/*.frag"
)

set(SPV_OUTPUTS "") # Initialize list of compiled shaders

# Loop through all found shaders and create compilation commands
foreach(SHADER_SOURCE_PATH ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER_SOURCE_PATH} NAME)
    set(SPV_OUTPUT_PATH "${SPV_OUTPUT_DIR}/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPV_OUTPUT_PATH}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SOURCE_PATH} -o ${SPV_OUTPUT_PATH}
        DEPENDS ${SHADER_SOURCE_PATH}
        COMMENT "Compiling ${SHADER_NAME} -> ${SHADER_NAME}.spv"
    )
    list(APPEND SPV_OUTPUTS ${SPV_OUTPUT_PATH})
endforeach()

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
