#include "Shader.h"
#include "Renderer/Vulkan/VulkanDevice.h"
#include "Core/AssetLocator.h"
#include "Core/Logger.h"
#include <fstream>
#include <algorithm>

namespace AstralEngine {

	static bool hasExtension(const std::string& path, const std::string& ext) {
		auto pos = path.find_last_of('.') ;
		if (pos == std::string::npos) return false;
		std::string e = path.substr(pos + 1);
		std::transform(e.begin(), e.end(), e.begin(), ::tolower);
		return e == ext;
	}

	static std::string toSpvPath(const std::string& path) {
		// Eğer zaten .spv ise aynen dön
		if (hasExtension(path, "spv")) return path;
		// shader.vert -> shader.vert.spv
		return path + ".spv";
	}

	static std::string resolveShaderPath(const std::string& path) {
		// Use AssetLocator for centralized path resolution
		return AssetLocator::getInstance().resolveShaderPath(path);
	}

	Shader::Shader(Vulkan::VulkanDevice& device, const std::string& vertFilepath, const std::string& fragFilepath)
		: m_device(device), m_hasFragmentShader(true) {
		// Path'leri çözümle ve SPIR-V yükle
		auto vertPath = toSpvPath(resolveShaderPath(vertFilepath));
		auto fragPath = toSpvPath(resolveShaderPath(fragFilepath));

		auto vertShaderCode = readFile(vertPath);
		auto fragShaderCode = readFile(fragPath);
		m_vertShaderModule = createShaderModule(vertShaderCode);
		m_fragShaderModule = createShaderModule(fragShaderCode);
	}
	
	Shader::Shader(Vulkan::VulkanDevice& device, const std::string& vertFilepath)
		: m_device(device), m_hasFragmentShader(false) {
		// Depth-only shader - path'i çözümle ve vertex shader'i yükle
		auto vertPath = toSpvPath(resolveShaderPath(vertFilepath));
		auto vertShaderCode = readFile(vertPath);
		m_vertShaderModule = createShaderModule(vertShaderCode);
		m_fragShaderModule = VK_NULL_HANDLE;
	}

	Shader::~Shader() {
		if (m_vertShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(m_device.getDevice(), m_vertShaderModule, nullptr);
		}
		if (m_fragShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(m_device.getDevice(), m_fragShaderModule, nullptr);
		}
	}

	std::vector<char> Shader::readFile(const std::string& filepath) {
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			// Use AssetLocator for detailed error reporting
			AE_ERROR("Failed to open shader file: %s", filepath.c_str());
			AssetLocator::getInstance().logSearchPaths();
			
			std::string errorMsg = "Failed to open shader file: " + filepath;
			errorMsg += "\nPlease check AssetLocator search paths in debug output above";
			errorMsg += "\nEnsure shaders are compiled with: compile_shaders.bat";
			throw std::runtime_error(errorMsg);
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();

		return buffer;
	}

	VkShaderModule Shader::createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_device.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}
}
