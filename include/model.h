#include "cgltf.h"
#include "glm/glm.hpp"
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <vector>
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"

class Model {
    public:
    
    cgltf_data* data;

    uint32_t index;

    glm::mat4 T;
    glm::mat4 R;
    glm::mat4 S;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;

    std::vector<uint32_t> indices;
    void load(const char* filepath);
    void loadImage();

};