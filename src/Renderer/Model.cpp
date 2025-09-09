#include "Renderer/Model.h"
#include "Core/Logger.h"
#include "Core/MemoryManager.h"
#include "Core/PerformanceMonitor.h"
#include "Core/AssetDependency.h"
#include "Renderer/Vulkan/VulkanDevice.h"
#include "Renderer/Vulkan/VulkanBuffer.h"
#include "Renderer/UnifiedMaterial.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <unordered_map>
#include <stdexcept>

namespace AstralEngine {

	Model::Model(Vulkan::VulkanDevice& device, const std::string& filepath)
		: m_device(device), m_filepath(filepath) {
		
		// Extract base directory using filesystem API for better path handling
		std::filesystem::path modelPath(filepath);
		m_baseDirectory = modelPath.parent_path().string();
		m_modelDirectory = m_baseDirectory; // Store for material loading
		
		// Initialize dependency graph (will be recreated in loadAsync if needed)
		m_dependencyGraph = std::make_unique<DependencyGraph>();
		
		// Load model geometry synchronously first
		loadModel(filepath);
		
		// Create default material as fallback
		createDefaultMaterial();
	}

	Model::~Model() = default;

	void Model::loadModel(const std::string& filepath) {
		PERF_TIMER("Model::loadModel");
		STACK_SCOPE(); // Use stack allocator for temporary model loading operations
		
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		// Use member variable for directory
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath.c_str(), m_baseDirectory.c_str(), true)) {
			AE_ERROR("Failed to load model '{}': {}", filepath, err);
			// Return early if model loading fails
			return;
		}

		// Convert tinyobj materials to our enhanced MaterialInfo format and store
		for (const auto& mat : materials) {
			MaterialLibrary::MaterialInfo info;
			info.name = mat.name;
			
			// Convert tinyobj material to our format
			info.baseColor = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
			info.emissive = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
			
			// Convert specular workflow to metallic-roughness
			info.metallic = mat.metallic; // Use PBR metallic if available
			info.roughness = mat.roughness > 0 ? mat.roughness : 
			                 (mat.shininess > 0 ? std::max(0.01f, 1.0f - (mat.shininess / 1000.0f)) : 1.0f);
			
			// Enhanced texture maps from tinyobj
			info.diffuseTexture = mat.diffuse_texname;
			info.normalTexture = mat.normal_texname.empty() ? mat.bump_texname : mat.normal_texname;
			info.metallicRoughnessTexture = mat.metallic_texname.empty() ? mat.roughness_texname : mat.metallic_texname;
			info.occlusionTexture = mat.ambient_texname; // Often used for AO
			info.emissiveTexture = mat.emissive_texname;
			
			info.isTransparent = (mat.dissolve < 1.0f);
			info.alphaCutoff = mat.dissolve;
			
			// Store in enhanced material info map
			m_materialInfos[info.name] = info;
			m_materialNameToBaseDir[info.name] = m_baseDirectory;
			
			// Extract texture paths and normalize them
			auto texPaths = MaterialLibrary::extractTexturePaths(info, m_baseDirectory);
			for (const auto& path : texPaths) {
				std::string normalizedPath = normalizeTexturePath(m_baseDirectory, 
					std::filesystem::path(path).filename().string());
				if (!normalizedPath.empty()) {
					m_texturePaths.insert(normalizedPath);
					m_textureDependencies.push_back(normalizedPath); // Keep legacy for now
				}
			}
			
			AE_DEBUG("Loaded material '{}' with {} texture dependencies", info.name, texPaths.size());
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		// Process shapes and create submeshes
		for (size_t shapeIdx = 0; shapeIdx < shapes.size(); ++shapeIdx) {
			const auto& shape = shapes[shapeIdx];
			uint32_t indexOffset = static_cast<uint32_t>(indices.size());
			uint32_t startVertexCount = static_cast<uint32_t>(vertices.size());
			
			// Get the most common material ID for this shape (simple heuristic)
			size_t shapeMaterialIdx = 0;
			if (!shape.mesh.material_ids.empty()) {
				// Use the first material ID as representative for the entire shape
				shapeMaterialIdx = shape.mesh.material_ids[0];
			}
			
			for (size_t faceIdx = 0; faceIdx < shape.mesh.indices.size(); faceIdx += 3) {
				// Process triangle indices
				for (int i = 0; i < 3; ++i) {
					const auto& index = shape.mesh.indices[faceIdx + i];
					Vertex vertex{};

					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};

					if (index.texcoord_index >= 0) {
						vertex.texCoord = {
							attrib.texcoords[2 * index.texcoord_index + 0],
							1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						};
					}

					if (index.normal_index >= 0) {
						vertex.normal = {
							attrib.normals[3 * index.normal_index + 0],
							attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
						};
					}

					vertex.color = {1.0f, 1.0f, 1.0f};

					if (uniqueVertices.count(vertex) == 0) {
						uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
						vertices.push_back(vertex);
					}
					indices.push_back(uniqueVertices[vertex]);
				}
			}
			
			// Create submesh with the shape's material
			uint32_t indexCount = static_cast<uint32_t>(indices.size()) - indexOffset;
			if (indexCount > 0) {
				std::string materialName = "default";
				if (shapeMaterialIdx < materials.size()) {
					materialName = materials[shapeMaterialIdx].name;
				}
				
				m_subMeshes.emplace_back(shape.name, materialName, indexOffset, indexCount);
				AE_DEBUG("Created submesh '{}' with material '{}': {} indices", 
					     shape.name, materialName, indexCount);
			}
		}

		if (!vertices.empty()) {
			createVertexBuffers();
		}
		if (!indices.empty()) {
			createIndexBuffers();
		}
	}

	void Model::createVertexBuffers() {
		PERF_TIMER("Model::createVertexBuffers");
		STACK_SCOPE(); // Use stack allocator for buffer operations
		
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		
		// Check if we have vertices to process
		if (bufferSize == 0 || vertices.empty()) {
			AE_WARN("No vertices to create vertex buffer");
			return;
		}

		Vulkan::VulkanBuffer stagingBuffer(m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(m_device.getAllocator(), stagingBuffer.getAllocation(), &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vmaUnmapMemory(m_device.getAllocator(), stagingBuffer.getAllocation());

		m_vertexBuffer = std::make_unique<Vulkan::VulkanBuffer>(m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		m_device.copyBuffer(stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize);
	}

	std::shared_ptr<UnifiedMaterialInstance> Model::getMaterial(const std::string& name) const {
		auto it = m_materials.find(name);
		if (it != m_materials.end()) return it->second;
		return m_defaultMaterial;
	}
	
	std::shared_ptr<UnifiedMaterialInstance> Model::getDefaultMaterial() const {
		return m_defaultMaterial;
	}
	
void Model::loadAsync(LoadCompleteCallback callback) {
		AE_DEBUG("Starting async material loading for model: {}", m_filepath);
		
		// Reset dependency graph for fresh loading
		m_dependencyGraph = std::make_unique<DependencyGraph>();
		
		// If no texture dependencies, just bind materials and complete
		if (m_texturePaths.empty()) {
			AE_INFO("No texture dependencies found, using materials without textures");
			try {
				createUnloadedMaterialInstances();
				bindMaterialsToSubmeshes();
				generateTangents();
				m_materialsReady = true;
				m_loadingComplete = true;
				callback(true, "");
			} catch (const std::exception& e) {
				callback(false, "Failed to create materials: " + std::string(e.what()));
			}
			return;
		}
		
		// Add texture dependencies to dependency graph
		for (const auto& texPath : m_texturePaths) {
			auto dep = std::make_shared<AssetDependency>(texPath, AssetDependency::Type::TEXTURE);
			m_dependencyGraph->addDependency(dep);
		}
		
		AE_INFO("Loading {} texture dependencies for model", m_texturePaths.size());
		
		// Start loading all texture dependencies
		m_dependencyGraph->loadDependencies(m_device, [this, callback](bool success, const std::string& error) {
			try {
				// Build loaded textures map regardless of success (some may have loaded)
				for (const auto& texPath : m_texturePaths) {
					auto texture = m_dependencyGraph->getTexture(texPath);
					if (texture) {
						m_loadedTextures[texPath] = texture;
						AE_DEBUG("Successfully loaded texture: {}", texPath);
					} else {
						AE_WARN("Failed to load texture: {}", texPath);
					}
				}
				
				AE_INFO("Loaded {}/{} textures, creating material instances", 
				        m_loadedTextures.size(), m_texturePaths.size());
				
				// Create material instances with loaded textures
				createLoadedMaterialInstances();
				
				// Bind materials to submeshes
				bindMaterialsToSubmeshes();
				
				// Calculate tangents for normal mapping
				generateTangents();
				
				// Mark as ready
				m_materialsReady = true;
				m_loadingComplete = true;
				
				// Report success even if some textures failed to load
				if (!success && m_loadedTextures.empty()) {
					// Only fail if no textures loaded at all
					callback(false, "Failed to load any textures: " + error);
				} else {
					callback(true, success ? "" : "Some textures failed to load: " + error);
				}
				
			} catch (const std::exception& e) {
				AE_ERROR("Exception during material loading: {}", e.what());
				callback(false, "Failed to process materials: " + std::string(e.what()));
			}
		});
	}
	
	bool Model::isFullyLoaded() const {
		return m_loadingComplete && (!m_dependencyGraph || m_dependencyGraph->isFullyLoaded());
	}
	
	float Model::getLoadingProgress() const {
		if (!m_dependencyGraph) return 1.0f;
		return m_dependencyGraph->getLoadingProgress();
	}
	
	void Model::createDefaultMaterial() {
		// Create a default material when none is specified
		m_defaultMaterial = std::make_shared<UnifiedMaterialInstance>();
		*m_defaultMaterial = UnifiedMaterialInstance::createDefault();
		m_defaultMaterial->setBaseColor(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
		m_defaultMaterial->setRoughness(0.7f);
		m_defaultMaterial->setMetallic(0.0f);
	}
	
	void Model::generateTangents() {
		// This is a simplified version - tangent space calculation is complex
		// TODO: Consider integrating MikkTSpace library for industry-standard tangent calculation
		// MikkTSpace is used by most modeling tools and ensures correct normal map appearance
		// across different applications. Current implementation may produce artifacts with
		// complex normal maps or models exported from certain tools.
		// In a real implementation, this would use a more robust algorithm
		
		// Skip if we don't have vertices or indices
		if (vertices.empty() || indices.empty()) return;
		
		// Process triangles
		for (size_t i = 0; i < indices.size(); i += 3) {
			if (i + 2 >= indices.size()) break;
			
			Vertex& v0 = vertices[indices[i]];
			Vertex& v1 = vertices[indices[i + 1]];
			Vertex& v2 = vertices[indices[i + 2]];
			
			// Edges of the triangle
			glm::vec3 edge1 = v1.position - v0.position;
			glm::vec3 edge2 = v2.position - v0.position;
			
			// Texture coordinate differences
			glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
			glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;
			
			// Handle zero UV deltas
			if (glm::length(deltaUV1) < 0.0001f) deltaUV1 = glm::vec2(1.0f, 0.0f);
			if (glm::length(deltaUV2) < 0.0001f) deltaUV2 = glm::vec2(0.0f, 1.0f);
			
			// Calculate tangent and bitangent
			float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
			glm::vec3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * r;
			glm::vec3 bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * r;
			
			// Ensure tangent is orthogonal to normal
			v0.tangent = tangent;
			v1.tangent = tangent;
			v2.tangent = tangent;
			
			v0.bitangent = bitangent;
			v1.bitangent = bitangent;
			v2.bitangent = bitangent;
		}
		
		// Normalize all tangents/bitangents
		for (auto& vertex : vertices) {
			vertex.tangent = glm::normalize(vertex.tangent);
			vertex.bitangent = glm::normalize(vertex.bitangent);
		}
	}

	void Model::loadMaterialsFromMTL(const std::string& mtlPath, const std::string& baseDir) {
		// Parse the MTL file
		std::vector<MaterialLibrary::MaterialInfo> materials;
		std::string error;
		
		if (!MaterialLibrary::parseMTL(mtlPath, materials, error)) {
			AE_WARN("Failed to parse MTL file {}: {}", mtlPath, error);
			return;
		}
		
		// Get loaded textures
		std::unordered_map<std::string, std::shared_ptr<Texture>> loadedTextures;
		if (m_dependencyGraph) {
			for (const auto& texPath : m_textureDependencies) {
				auto tex = m_dependencyGraph->getTexture(texPath);
				if (tex) {
					loadedTextures[texPath] = tex;
				}
			}
		}
		
		// Create materials
		for (const auto& info : materials) {
			m_materials[info.name] = MaterialLibrary::createMaterial(info, baseDir, loadedTextures);
		}
		
		AE_INFO("Created {} materials from MTL file {}", materials.size(), mtlPath);
	}

	void Model::createIndexBuffers() {
		PERF_TIMER("Model::createIndexBuffers");
		STACK_SCOPE(); // Use stack allocator for buffer operations
		
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		
		// Check if we have indices to process
		if (bufferSize == 0 || indices.empty()) {
			AE_WARN("No indices to create index buffer");
			return;
		}

		Vulkan::VulkanBuffer stagingBuffer(m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

		void* data;
		vmaMapMemory(m_device.getAllocator(), stagingBuffer.getAllocation(), &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vmaUnmapMemory(m_device.getAllocator(), stagingBuffer.getAllocation());

		m_indexBuffer = std::make_unique<Vulkan::VulkanBuffer>(m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

		m_device.copyBuffer(stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize);
	}

	void Model::Bind(VkCommandBuffer commandBuffer) {
    if (m_vertexBuffer && m_indexBuffer) {
        VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    }
}

void Model::Draw(VkCommandBuffer commandBuffer) {
    if (m_indexBuffer && !indices.empty()) {
        // If we have submeshes, draw them individually (materials may differ)
        if (!m_subMeshes.empty()) {
            for (const auto& sm : m_subMeshes) {
                vkCmdDrawIndexed(commandBuffer, sm.indexCount, 1, sm.indexOffset, 0, 0);
            }
        } else {
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        }
    }
}

void Model::DrawSubMesh(VkCommandBuffer commandBuffer, size_t submeshIndex) {
    if (submeshIndex >= m_subMeshes.size()) return;
    const auto& sm = m_subMeshes[submeshIndex];
    vkCmdDrawIndexed(commandBuffer, sm.indexCount, 1, sm.indexOffset, 0, 0);
}

	std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions() {
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
		// Altı özelliğimiz var: position, color, normal, texCoord, tangent, bitangent
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(6);

		// Konum (Position) -> layout(location = 0)
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // glm::vec3
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		// Renk (Color) -> layout(location = 1)
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // glm::vec3
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		// Normal -> layout(location = 2)
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT; // glm::vec3
		attributeDescriptions[2].offset = offsetof(Vertex, normal);

		// Doku Koordinatı (TexCoord) -> layout(location = 3)
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT; // glm::vec2
		attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

		// Add tangent and bitangent for PBR rendering
		attributeDescriptions.resize(6); // Position, Color, Normal, TexCoord, Tangent, Bitangent
		
		// Tangent -> layout(location = 4)
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT; // glm::vec3
		attributeDescriptions[4].offset = offsetof(Vertex, tangent);
		
		// Bitangent -> layout(location = 5)
		attributeDescriptions[5].binding = 0;
		attributeDescriptions[5].location = 5;
		attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT; // glm::vec3
		attributeDescriptions[5].offset = offsetof(Vertex, bitangent);

		return attributeDescriptions;
	}

	// Implementation of new enhanced API methods
	const std::unordered_map<std::string, std::shared_ptr<UnifiedMaterialInstance>>& Model::getMaterials() const {
		return m_materials;
	}

	std::shared_ptr<UnifiedMaterialInstance> Model::getSubmeshMaterial(uint32_t index) const {
		if (index >= m_subMeshes.size()) {
			return m_defaultMaterial;
		}

		const auto& submesh = m_subMeshes[index];
		if (submesh.material) {
			return submesh.material;
		}

		// Try to find by material name
		auto it = m_materials.find(submesh.materialName);
		if (it != m_materials.end()) {
			return it->second;
		}

		// Fallback to default
		return m_defaultMaterial;
	}

	bool Model::areMaterialsReady() const {
		return m_materialsReady.load();
	}

	float Model::getMaterialLoadingProgress() const {
		if (!m_dependencyGraph) {
			return 1.0f;
		}
		return m_dependencyGraph->getLoadingProgress();
	}
	
	size_t Model::getSubmeshCount() const {
		return m_subMeshes.size();
	}
	
	const SubMesh& Model::getSubmesh(size_t index) const {
		if (index >= m_subMeshes.size()) {
			throw std::out_of_range("Submesh index out of range");
		}
		return m_subMeshes[index];
	}

	// Path normalization utility
	std::string Model::normalizeTexturePath(const std::string& basePath, const std::string& relativePath) const {
		if (relativePath.empty()) {
			return "";
		}

		try {
			std::filesystem::path base(basePath);
			std::filesystem::path relative(relativePath);
			std::filesystem::path combined = base / relative;
			
			// Return canonical path if possible, otherwise weakly_canonical
			if (std::filesystem::exists(combined)) {
				return std::filesystem::canonical(combined).string();
			} else {
				return std::filesystem::weakly_canonical(combined).string();
			}
		} catch (const std::filesystem::filesystem_error& e) {
			AE_WARN("Failed to normalize texture path '{}' + '{}': {}", basePath, relativePath, e.what());
			return (std::filesystem::path(basePath) / relativePath).string();
		}
	}

	// Create material instances with loaded textures
	void Model::createLoadedMaterialInstances() {
		AE_DEBUG("Creating material instances with loaded textures");
		
		for (const auto& [materialName, materialInfo] : m_materialInfos) {
			// Get base directory for this material
			std::string baseDir = m_materialNameToBaseDir[materialName];
			
			// Build texture map for this material using loaded textures
			std::unordered_map<std::string, std::shared_ptr<Texture>> materialTextures;
			
			// Helper to add texture if it exists
			auto addTexture = [&](const std::string& relativePath) {
				if (!relativePath.empty()) {
					std::string normalizedPath = normalizeTexturePath(baseDir, relativePath);
					auto it = m_loadedTextures.find(normalizedPath);
					if (it != m_loadedTextures.end()) {
						// MaterialLibrary expects key as baseDir + "/" + relativePath
						std::string key = baseDir + "/" + relativePath;
						materialTextures[key] = it->second;
						AE_DEBUG("Added texture '{}' for material '{}'", key, materialName);
					}
				}
			};
			
			// Add all possible textures
			addTexture(materialInfo.diffuseTexture);
			addTexture(materialInfo.normalTexture);
			addTexture(materialInfo.metallicRoughnessTexture);
			addTexture(materialInfo.occlusionTexture);
			addTexture(materialInfo.emissiveTexture);
			
			// Create material instance
			m_materials[materialName] = MaterialLibrary::createMaterial(materialInfo, baseDir, materialTextures);
			AE_INFO("Created material '{}' with {} textures", materialName, materialTextures.size());
		}
	}

	// Create material instances without textures
	void Model::createUnloadedMaterialInstances() {
		AE_DEBUG("Creating material instances without textures");
		
		for (const auto& [materialName, materialInfo] : m_materialInfos) {
			// Get base directory for this material
			std::string baseDir = m_materialNameToBaseDir[materialName];
			
			// Empty texture map - materials will use default values
			std::unordered_map<std::string, std::shared_ptr<Texture>> emptyTextures;
			
			// Create material instance
			m_materials[materialName] = MaterialLibrary::createMaterial(materialInfo, baseDir, emptyTextures);
			AE_INFO("Created material '{}' without textures", materialName);
		}
	}

	// Bind created materials to submeshes
	void Model::bindMaterialsToSubmeshes() {
		AE_DEBUG("Binding materials to submeshes");
		
		for (auto& submesh : m_subMeshes) {
			auto it = m_materials.find(submesh.materialName);
			if (it != m_materials.end()) {
				submesh.material = it->second;
				AE_DEBUG("Bound material '{}' to submesh '{}'", submesh.materialName, submesh.name);
			} else {
				submesh.material = m_defaultMaterial;
				AE_WARN("Material '{}' not found for submesh '{}', using default", 
				        submesh.materialName, submesh.name);
			}
		}
	}

} // namespace AstralEngine
