#include "Asset/ImageAssetManager.h"
#include "Core/Logger.h"
#include "Renderer/Texture.h"
#include "Renderer/RRenderer.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

// Include STB image for image loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// Include STB image write for image saving
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

namespace AstralEngine {
    namespace Asset {
        std::shared_ptr<Texture> ImageAssetManager::loadImage(const std::string& filepath) {
            // Detect format
            ImageFormat format = detectFormat(filepath);
            
            // Load based on format
            switch (format) {
                case ImageFormat::PNG:
                    return loadPNG(filepath);
                case ImageFormat::JPEG:
                    return loadJPEG(filepath);
                case ImageFormat::BMP:
                    return loadBMP(filepath);
                case ImageFormat::TIFF:
                    return loadTIFF(filepath);
                default:
                    AE_WARN("Desteklenmeyen görüntü formatı: {}", filepath);
                    return nullptr;
            }
        }
        
        void ImageAssetManager::saveImage(const std::string& filepath, const Texture& texture, ImageFormat format) {
            // Save based on format
            switch (format) {
                case ImageFormat::PNG:
                    savePNG(filepath, texture);
                    break;
                case ImageFormat::JPEG:
                    saveJPEG(filepath, texture);
                    break;
                case ImageFormat::BMP:
                    saveBMP(filepath, texture);
                    break;
                default:
                    AE_WARN("Desteklenmeyen görüntü formatı: {}", filepath);
                    break;
            }
        }
        
        std::shared_ptr<Project> ImageAssetManager::loadProject(const std::string& filepath) {
            // Detect format
            ImageFormat format = detectFormat(filepath);
            
            // Load project based on format
            if (format == ImageFormat::PSD) {
                return loadPSD(filepath);
            } else {
                AE_WARN("Desteklenmeyen proje formatı: {}", filepath);
                return nullptr;
            }
        }
        
        void ImageAssetManager::saveProject(const std::string& filepath, const Project& project) {
            // Detect format
            ImageFormat format = detectFormat(filepath);
            
            // Save project based on format
            if (format == ImageFormat::PSD) {
                savePSD(filepath, project);
            } else {
                AE_WARN("Desteklenmeyen proje formatı: {}", filepath);
            }
        }
        
        void ImageAssetManager::copyToClipboard(const Texture& texture) {
            // For now, we'll just log the operation
            // In a full implementation, we would copy the texture data to the system clipboard
            AE_DEBUG("Texture clipboard'a kopyalandı");
        }
        
        std::shared_ptr<Texture> ImageAssetManager::pasteFromClipboard() {
            // For now, we'll just log the operation and return nullptr
            // In a full implementation, we would paste texture data from the system clipboard
            AE_DEBUG("Clipboard'dan texture yapıştırıldı");
            return nullptr;
        }
        
        ImageFormat ImageAssetManager::detectFormat(const std::string& filepath) {
            std::string ext = getExtension(filepath);
            
            // Convert to lowercase for comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == "png") {
                return ImageFormat::PNG;
            } else if (ext == "jpg" || ext == "jpeg") {
                return ImageFormat::JPEG;
            } else if (ext == "bmp") {
                return ImageFormat::BMP;
            } else if (ext == "tiff" || ext == "tif") {
                return ImageFormat::TIFF;
            } else if (ext == "psd") {
                return ImageFormat::PSD;
            } else {
                return ImageFormat::Custom;
            }
        }
        
        std::shared_ptr<Texture> ImageAssetManager::loadPNG(const std::string& filepath) {
            // Load PNG using stb_image
            int width, height, channels;
            unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            
            if (!data) {
                AE_ERROR("PNG dosyası yüklenemedi: {}", filepath);
                return nullptr;
            }
            
            AE_DEBUG("PNG dosyası yüklendi: {} ({}x{}, {} kanal)", filepath, width, height, channels);
            
            // For now, we'll just free the data and return nullptr
            // In a full implementation, we would create a Texture object from the data
            stbi_image_free(data);
            
            return nullptr;
        }
        
        std::shared_ptr<Texture> ImageAssetManager::loadJPEG(const std::string& filepath) {
            // Load JPEG using stb_image
            int width, height, channels;
            unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            
            if (!data) {
                AE_ERROR("JPEG dosyası yüklenemedi: {}", filepath);
                return nullptr;
            }
            
            AE_DEBUG("JPEG dosyası yüklendi: {} ({}x{}, {} kanal)", filepath, width, height, channels);
            
            // For now, we'll just free the data and return nullptr
            // In a full implementation, we would create a Texture object from the data
            stbi_image_free(data);
            
            return nullptr;
        }
        
        std::shared_ptr<Texture> ImageAssetManager::loadBMP(const std::string& filepath) {
            // Load BMP using stb_image
            int width, height, channels;
            unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            
            if (!data) {
                AE_ERROR("BMP dosyası yüklenemedi: {}", filepath);
                return nullptr;
            }
            
            AE_DEBUG("BMP dosyası yüklendi: {} ({}x{}, {} kanal)", filepath, width, height, channels);
            
            // For now, we'll just free the data and return nullptr
            // In a full implementation, we would create a Texture object from the data
            stbi_image_free(data);
            
            return nullptr;
        }
        
        std::shared_ptr<Texture> ImageAssetManager::loadTIFF(const std::string& filepath) {
            // Load TIFF using stb_image
            int width, height, channels;
            unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            
            if (!data) {
                AE_ERROR("TIFF dosyası yüklenemedi: {}", filepath);
                return nullptr;
            }
            
            AE_DEBUG("TIFF dosyası yüklendi: {} ({}x{}, {} kanal)", filepath, width, height, channels);
            
            // For now, we'll just free the data and return nullptr
            // In a full implementation, we would create a Texture object from the data
            stbi_image_free(data);
            
            return nullptr;
        }
        
        void ImageAssetManager::savePNG(const std::string& filepath, const Texture& texture) {
            // For now, we'll just log the operation
            // In a full implementation, we would use stb_image_write to save the PNG file
            AE_DEBUG("PNG dosyası kaydediliyor: {}", filepath);
        }
        
        void ImageAssetManager::saveJPEG(const std::string& filepath, const Texture& texture) {
            // For now, we'll just log the operation
            // In a full implementation, we would use stb_image_write to save the JPEG file
            AE_DEBUG("JPEG dosyası kaydediliyor: {}", filepath);
        }
        
        void ImageAssetManager::saveBMP(const std::string& filepath, const Texture& texture) {
            // For now, we'll just log the operation
            // In a full implementation, we would use stb_image_write to save the BMP file
            AE_DEBUG("BMP dosyası kaydediliyor: {}", filepath);
        }
        
        std::shared_ptr<Project> ImageAssetManager::loadPSD(const std::string& filepath) {
            // For now, we'll just log the operation and return nullptr
            // In a full implementation, we would parse the PSD file format
            AE_DEBUG("PSD dosyası yükleniyor: {}", filepath);
            
            // TODO: Implement actual PSD loading
            
            return nullptr;
        }
        
        void ImageAssetManager::savePSD(const std::string& filepath, const Project& project) {
            // For now, we'll just log the operation
            // In a full implementation, we would serialize the project to PSD format
            AE_DEBUG("PSD dosyası kaydediliyor: {}", filepath);
            
            // TODO: Implement actual PSD saving
        }
        
        std::string ImageAssetManager::getExtension(const std::string& filepath) {
            std::string ext = std::filesystem::path(filepath).extension().string();
            if (!ext.empty() && ext[0] == '.') {
                return ext.substr(1); // Baştaki noktayı kaldır.
            }
            return ext;
        }
    }
}