#include "ECSTest.h"
#include "ECS.h"
#include "Core/Logger.h"
#include "Core/MemoryManager.h"
#include "Core/PerformanceMonitor.h"

namespace AstralEngine::ECS {

void demonstrateNewECSSystem() {
    PERF_TIMER("demonstrateNewECSSystem");
    STACK_SCOPE(); // Use stack allocator for ECS demonstration
    
    AE_INFO("=== New ECS System Demonstration ===");
    
    // Create a scene (which contains a registry and systems)
    Scene scene;
    
    AE_INFO("Created scene with {} entities and {} systems", 
            scene.getEntityCount(), scene.getSystemCount());
    
    // Create some entities
    EntityID camera = scene.createEntity("MainCamera");
    EntityID cube1 = scene.createEntity("Cube1");
    EntityID cube2 = scene.createEntity("Cube2");
    EntityID cube3 = scene.createEntity("Cube3");
    
    AE_INFO("Created {} entities", scene.getEntityCount());
    
    // Set up camera
    scene.addComponent<Transform>(camera, glm::vec3(0, 0, 5));
    scene.addComponent<Camera>(camera, 45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    scene.setMainCamera(camera);
    
    // Set up cube entities with hierarchical transforms
    auto& transform1 = scene.addComponent<Transform>(cube1, glm::vec3(0, 0, 0));
    auto& transform2 = scene.addComponent<Transform>(cube2, glm::vec3(2, 0, 0));
    auto& transform3 = scene.addComponent<Transform>(cube3, glm::vec3(0, 2, 0));
    
    // Set up parent-child relationships
    transform2.parent = cube1;
    transform1.children.push_back(cube2);
    
    transform3.parent = cube2;
    transform2.children.push_back(cube3);
    
    AE_INFO("Set up transform hierarchy: cube1 -> cube2 -> cube3");
    
    // Add some components for testing
    scene.addComponent<Name>(cube1, "FirstCube");
    scene.addComponent<Tag>(cube1, "Renderable");
    scene.addComponent<Tag>(cube2, "Movable");
    
    // Test component queries
    AE_INFO("=== Component Queries ===");
    
    // Query entities with Transform component
    auto transformView = scene.view<Transform>();
    AE_INFO("Entities with Transform: {}", transformView.size());
    
    for (auto [entity, transform] : transformView) {
        const std::string& name = scene.getEntityName(entity);
        AE_INFO("Entity {} ('{}'): position({:.1f}, {:.1f}, {:.1f})", 
                entity, name, transform.position.x, transform.position.y, transform.position.z);
    }
    
    // Query entities with both Transform and Camera
    auto cameraView = scene.view<Transform, Camera>();
    AE_INFO("Entities with Transform + Camera: {}", cameraView.size());
    
    // Query entities with Name component
    auto nameView = scene.view<Name>();
    AE_INFO("Entities with Name: {}", nameView.size());
    for (auto [entity, name] : nameView) {
        AE_INFO("Entity {}: name = '{}'", entity, name.name);
    }
    
    // Query entities with Tag component
    auto tagView = scene.view<Tag>();
    AE_INFO("Entities with Tag: {}", tagView.size());
    for (auto [entity, tag] : tagView) {
        AE_INFO("Entity {}: tag = '{}'", entity, tag.tag);
    }
    
    // Simulate a frame update
    AE_INFO("=== Frame Update ===");
    
    // Move cube1 (should affect its children through hierarchy)
    transform1.position.x += 1.0f;
    transform1.markDirty();
    
    // Update scene (runs all systems)
    float deltaTime = 0.016f; // 60 FPS
    scene.update(deltaTime);
    
    // Check world matrices after transform update
    AE_INFO("=== World Matrices After Update ===");
    for (auto [entity, transform] : transformView) {
        glm::mat4 worldMatrix = transform.getWorldMatrix(scene.getRegistry());
        glm::vec4 worldPos = worldMatrix * glm::vec4(0, 0, 0, 1);
        const std::string& name = scene.getEntityName(entity);
        AE_INFO("Entity {} ('{}'): world position({:.1f}, {:.1f}, {:.1f})", 
                entity, name, worldPos.x, worldPos.y, worldPos.z);
    }
    
    // Test component management
    AE_INFO("=== Component Management ===");
    AE_INFO("Camera has Transform: {}", scene.hasComponent<Transform>(camera));
    AE_INFO("Camera has Camera: {}", scene.hasComponent<Camera>(camera));
    AE_INFO("Cube1 has Name: {}", scene.hasComponent<Name>(cube1));
    AE_INFO("Cube2 has Name: {}", scene.hasComponent<Name>(cube2));
    
    // Remove a component
    scene.removeComponent<Camera>(camera);
    AE_INFO("After removing Camera component:");
    AE_INFO("Camera has Camera: {}", scene.hasComponent<Camera>(camera));
    
    // Test system management
    AE_INFO("=== System Management ===");
    AE_INFO("Current systems: {}", scene.getSystemCount());
    
    auto& transformSys = scene.getTransformSystem();
    auto& renderSys = scene.getRenderSystem();
    AE_INFO("Transform System found: {}", transformSys.getName());
    AE_INFO("Render System found: {}", renderSys.getName());
    
    // Print comprehensive debug info
    scene.printDebugInfo();
    
    // Clean up test
    scene.destroyEntity(camera);
    scene.destroyEntity(cube1);
    scene.destroyEntity(cube2);
    scene.destroyEntity(cube3);
    
    AE_INFO("Entities after cleanup: {}", scene.getEntityCount());
    AE_INFO("=== New ECS System Demonstration Complete ===");
}

} // namespace AstralEngine::ECS
