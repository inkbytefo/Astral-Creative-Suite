cmake_minimum_required(VERSION 3.16)
project(AstralEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- SDL3 Entegrasyonu ---
set(SDL3_DIR ${PROJECT_SOURCE_DIR}/external/SDL3/cmake)
find_package(SDL3 REQUIRED CONFIG)

# --- Vulkan Entegrasyonu ---
find_package(Vulkan REQUIRED)

# --- Kaynak Kodunu ve Kütüphaneyi Tanımla ---
add_subdirectory(src)

# --- Kütüphanenin Bağımlılıklarını Belirt ---
# PUBLIC: Bu kütüphaneyi kullanan diğer hedeflerin de bu yolları ve kütüphaneleri görmesini sağlar.
# Bu tek komut, hem AstralEngine'in hem de AstralEditor'ın ihtiyaç duyduğu her şeyi bağlar.
target_link_libraries(AstralEngine PUBLIC
    Vulkan::Vulkan
    SDL3::SDL3
)

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

# --- Derleme Sonrası Adımlar ---
add_custom_command(
    TARGET AstralEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${PROJECT_SOURCE_DIR}/external/SDL3/lib/x64/SDL3.dll
    $<TARGET_FILE_DIR:AstralEditor>
    COMMENT "SDL3.dll kopyalanıyor..."
)

# Assets klasörünü çıktı dizinine kopyala
add_custom_command(
    TARGET AstralEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:AstralEditor>/assets"
    COMMENT "Assets kopyalanıyor..."
)

# Shaders klasörünü çıktı dizinine kopyala
add_custom_command(
    TARGET AstralEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:AstralEditor>/shaders"
    COMMENT "Shaders kopyalanıyor..."
)
