#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include "GLFW/glfw3.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <sys/types.h>
#include <vector>

typedef struct  {
    int graphicsQueueIndex;
    VkQueue queue;
} GraphicsQueue;