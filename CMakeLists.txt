cmake_minimum_required(VERSION 3.16)
project(AstralEngine VERSION 1.0 LANGUAGES CXX)

# --- BU KODU EKLEYİN ---
# Tüm hedeflerin aynı MSVC runtime kütüphanesini kullanmasını sağla
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
# --- EKLEME SONU ---

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- SDL3 Entegrasyonu ---
set(SDL3_DIR ${PROJECT_SOURCE_DIR}/external/SDL3/cmake)
find_package(SDL3 REQUIRED CONFIG)

# --- Vulkan Entegrasyonu ---
find_package(Vulkan 1.4 REQUIRED)

# --- Harici Kütüphaneleri Tanımla ---
add_subdirectory(external/tinyobjloader)

# shaderc'nin runtime derlemesi KALDIRILDI (tek kaynak: offline SPIR-V)
# set(SHADERC_SKIP_TESTS ON)
# add_subdirectory(external/shaderc)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${PROJECT_SOURCE_DIR}/external/stb)

# --- Kaynak Kodunu ve Kütüphaneyi Tanımla ---
add_subdirectory(src)

# --- Kütüphanenin Bağımlılıklarını Belirt ---
# PUBLIC: Bu kütüphaneyi kullanan diğer hedeflerin de bu yolları ve kütüphaneleri görmesini sağlar.
# Bu tek komut, hem AstralEngine'in hem de AstralEditor'ın ihtiyaç duyduğu her şeyi bağlar.
target_link_libraries(AstralEngine PUBLIC
    Vulkan::Vulkan
    SDL3::SDL3
    tinyobjloader
    stb_image
)

# GLM'in deneysel özelliklerini kullanmak için bu tanımı ekleyin
target_compile_definitions(AstralEngine PUBLIC GLM_ENABLE_EXPERIMENTAL)

target_include_directories(AstralEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external/glm
    ${PROJECT_SOURCE_DIR}/external/tinyobjloader
    ${PROJECT_SOURCE_DIR}/external/vma/include # VMA için 'include' alt klasörünü belirtiyoruz
)

# --- Çalıştırılabilir Dosyayı Tanımla ---
add_executable(AstralEditor src/main.cpp)

# --- Çalıştırılabilir Dosyanın Bağımlılıklarını Belirt ---
target_link_libraries(AstralEditor PRIVATE AstralEngine)

# --- Platforma Bağımlı Derleme Sonrası Adımlar ---
if(WIN32)
    # Windows için DLL'leri kopyala
    add_custom_command(
        TARGET AstralEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_SOURCE_DIR}/external/SDL3/lib/x64/SDL3.dll
        $<TARGET_FILE_DIR:AstralEditor>
        COMMENT "SDL3.dll kopyalanıyor..."
    )
endif()

# Assets klasörünü çıktı dizinine kopyala
add_custom_command(
    TARGET AstralEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:AstralEditor>/assets"
    COMMENT "Assets kopyalanıyor..."
)

# Check for glslc (Vulkan SDK shader compiler)
find_program(GLSLC_EXECUTABLE glslc)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please install Vulkan SDK with glslc compiler.")
else()
    message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")
endif()

# Otomatik Shader Derleme Sistemi
# 1. Shader dosyalarını kaynak dizinden bul
file(GLOB SHADER_SOURCES 
    "${PROJECT_SOURCE_DIR}/shaders/*.vert" 
    "${PROJECT_SOURCE_DIR}/shaders/*.frag"
)

# 2. Her shader dosyası için döngüyle SPV dosyası oluştur
set(SPV_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders_spv")
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(FILE_NAME ${SHADER} NAME)          # shader.vert
    set(SPV "${SPV_OUTPUT_DIR}/${FILE_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SPV_OUTPUT_DIR}"
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SPV}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${FILE_NAME} -> ${FILE_NAME}.spv"
    )

    list(APPEND SPV_OUTPUTS ${SPV})
endforeach()

# 3. Shader derlemeyi AstralEditor hedefine bağla
add_custom_target(Shaders ALL DEPENDS ${SPV_OUTPUTS})
add_dependencies(AstralEditor Shaders)

# 4. Derlenen SPV'leri çıktı dizinine kopyala
add_custom_command(
    TARGET AstralEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:AstralEditor>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SPV_OUTPUT_DIR}" "$<TARGET_FILE_DIR:AstralEditor>/shaders"
    COMMENT "Compiled SPIR-V shaderlar kopyalanıyor..."
)

# Kaynak GLSL'leri opsiyonel olarak çıktı dizinine kopyala (geliştirme için)
add_custom_command(
    TARGET AstralEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:AstralEditor>/shaders_src"
    COMMENT "GLSL kaynak shaderlar (opsiyonel) kopyalanıyor..."
)

# Pipeline cache dosyasını çıktı dizinine kopyala (sadece varsa)
if(EXISTS "${PROJECT_SOURCE_DIR}/pipeline_cache.bin")
    add_custom_command(
        TARGET AstralEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/pipeline_cache.bin" "$<TARGET_FILE_DIR:AstralEditor>/pipeline_cache.bin"
        COMMENT "Pipeline cache kopyalanıyor..."
    )
endif()
