#pragma once

// Main renderer components
#include "Renderer.h"
#include "RenderSystem.h"
#include "2D/CanvasRenderer.h"
#include "3D/SceneRenderer.h"

// Vulkan components
#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanUtils.h"
#include "Vulkan/DescriptorSetManager.h"

// Material and shader components
#include "UnifiedMaterial.h"
#include "UnifiedMaterialConstants.h"
#include "MaterialShaderManager.h"
#include "Shader.h"
#include "ShaderHotReload.h"
#include "PipelineConfig.h"
#include "Texture.h"
#include "Model.h"

// Additional components
#include "Camera.h"
#include "ShadowMapping.h"

// Namespace alias for backward compatibility
namespace AstralRenderer = AstralEngine;