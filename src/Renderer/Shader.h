#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>
// Forward declaration
namespace AstralEngine {
    namespace Vulkan {
        class VulkanDevice;
    }
}

namespace AstralEngine {
	class Shader {
	public:
		// Not: vertFilepath/fragFilepath için .vert/.frag verilirse, çalışma zamanında .spv uzantısı eklenerek okunur.
		Shader(Vulkan::VulkanDevice& device, const std::string& vertFilepath, const std::string& fragFilepath);
		
		// Depth-only shader constructor (vertex shader only)
		Shader(Vulkan::VulkanDevice& device, const std::string& vertFilepath);
		
		~Shader();

		VkShaderModule getVertShaderModule() const { return m_vertShaderModule; }
		VkShaderModule getFragShaderModule() const { return m_fragShaderModule; }
		
		// Check if shader has fragment stage
		bool hasFragmentShader() const { return m_hasFragmentShader; }

	private:
		static std::vector<char> readFile(const std::string& filepath);
		VkShaderModule createShaderModule(const std::vector<char>& code);

		Vulkan::VulkanDevice& m_device;
		VkShaderModule m_vertShaderModule;
		VkShaderModule m_fragShaderModule;
		bool m_hasFragmentShader;
	};
}
