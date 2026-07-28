#include "cgltf.h"
#include "glm/glm.hpp"
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <vector>
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"

class Model {
    public:
    
    uint32_t index;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;

    std::vector<uint32_t> indices;
    Model(const char* filepath);



};