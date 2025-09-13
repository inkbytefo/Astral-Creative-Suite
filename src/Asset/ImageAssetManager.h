#pragma once

#include "Core/AssetManager.h"
#include "2D/Layers/Layer.h"
#include <string>
#include <vector>

namespace AstralEngine {
    namespace Asset {
        // Supported image formats
        enum class ImageFormat {
            PNG,
            JPEG,
            BMP,
            TIFF,
            PSD,
            Custom
        };
        
        // Project structure
        struct Project {
            std::string name;
            std::string filePath;
            uint32_t width;
            uint32_t height;
            std::vector<D2::Layer> layers;
            // Add more project properties as needed
        };
        
        // Enhanced asset manager for 2D formats
        class ImageAssetManager {
        public:
            ImageAssetManager() = default;
            ~ImageAssetManager() = default;
            
            // Load various image formats
            std::shared_ptr<Texture> loadImage(const std::string& filepath);
            void saveImage(const std::string& filepath, const Texture& texture, ImageFormat format);
            
            // Layered format support (PSD-like)
            std::shared_ptr<Project> loadProject(const std::string& filepath);
            void saveProject(const std::string& filepath, const Project& project);
            
            // Clipboard operations
            void copyToClipboard(const Texture& texture);
            std::shared_ptr<Texture> pasteFromClipboard();
            
            // Format detection
            ImageFormat detectFormat(const std::string& filepath);
            
        private:
            // Format-specific loaders
            std::shared_ptr<Texture> loadPNG(const std::string& filepath);
            std::shared_ptr<Texture> loadJPEG(const std::string& filepath);
            std::shared_ptr<Texture> loadBMP(const std::string& filepath);
            std::shared_ptr<Texture> loadTIFF(const std::string& filepath);
            
            // Format-specific savers
            void savePNG(const std::string& filepath, const Texture& texture);
            void saveJPEG(const std::string& filepath, const Texture& texture);
            void saveBMP(const std::string& filepath, const Texture& texture);
            
            // PSD support (layered format)
            std::shared_ptr<Project> loadPSD(const std::string& filepath);
            void savePSD(const std::string& filepath, const Project& project);
            
            // Helper methods
            std::string getExtension(const std::string& filepath);
        };
    }
}