#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include "GLFW/glfw3.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include "glm/glm.hpp"

#define TINYGLTF_IMPLEMENTATION

#define TINYGLTF3_ENABLE_FS

typedef struct  {
    int graphicsQueueIndex;
    VkQueue queue;
} GraphicsQueue;

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};