#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include "GLFW/glfw3.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <sys/types.h>
#include <vector>
#include "glm/glm.hpp"

typedef struct  {
    int graphicsQueueIndex;
    VkQueue queue;
} GraphicsQueue;

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
};

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};