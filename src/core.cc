#include "../include/core.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <glm/ext/matrix_float4x4.hpp>
#include <limits>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/glm.hpp"
#include "../include/model.h"

#define CGLTF_IMPLEMENTATION
#include "../include/cgltf.h"

class Core {
    public:
    GLFWwindow* window;
    const int WIDTH = 1000; 
    const int HEIGHT = 700; 
    const int MAX_FRAMES_IN_FLIGHT = 2;
    VkInstance instance;

    VkSurfaceKHR surface;
    VkExtent2D extent;
    VkFormat format;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    GraphicsQueue queue;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorLayout;
    std::vector<VkDescriptorSetLayout> layouts;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkBuffer> uboBuffers;
    std::vector<VkDeviceMemory> uboMemories;
    std::vector<void*> uboDatas;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkDeviceMemory indexBufferMemory;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t> indices;

    std::vector<VkSemaphore> presentCompleteSemaphores;
    std::vector<VkSemaphore> renderedFinishedSemaphores;
    std::vector<VkFence> drawFences;

    int currentFrame = 0;
    uint32_t imageIndex;

    std::vector<VkCommandBuffer> commandBuffers;
    VkCommandPool commandPool;

    UniformBufferObject ubo{};

    glm::vec3 cameraPos{0.0f, 0.0f, 3.0f};
    glm::vec3 cameraFront{0.0f, 0.0f, -1.0f};
    glm::vec3 cameraUp{0.0f, 1.0f, 0.0f};
    float yaw = -90.0f, pitch = 0.0f;

    double lastFrameTime = 0.0;
    float deltaTime = 0.0f;

    double xpos;
    double ypos;

    std::vector<Model> models; // this will hold vertex and index buffer, index in the ssbo
    std::vector<glm::mat4> instances; // this is the model matrices, should be placed in the SSBO

    void runApp() {
        initEngine();
    }
    void initEngine() {
        createWindow();
        createInstance();
        createSurface();
        createDevice();
        createSwapchain();
        createSwapchainImages();
        loadModels();
        createDescriptorPool();
        allocateDescriptorSets();
        createPipeline();
        createVertexAndIndexBuffers();
        createSyncObjects();
        createCommandBufferPool();
        allocateCommandBuffers();
        mainLoop();

    }

    void loadModels() {

        Model model("../assets/jax.glb");
        

        printf("sizes: %zu %zu %zu\n", vertices.size(), normals.size(), uvs.size());
        
            
        
        
    }

    void createWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "vk engine", nullptr, nullptr);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);    
        glfwSetCursorPos(window, 0, 0);

    }
    void createInstance() {

        const char* validation_layers[] = {
            "VK_LAYER_KHRONOS_validation"
        };

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "vk";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "engine";

        uint32_t glfwExtensionsCount;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);

        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.enabledExtensionCount = extensions.size();
        instanceInfo.ppEnabledExtensionNames = extensions.data();
        instanceInfo.enabledLayerCount = 1;
        instanceInfo.ppEnabledLayerNames = validation_layers;

        if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance");
        } else {
            printf("instance created!\n");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice pdev) {
        VkPhysicalDeviceProperties deviceProps; 
        vkGetPhysicalDeviceProperties(pdev, &deviceProps);
        if (deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            return true;
        }
        return false;
    }

    void createDevice() {
        std::vector<VkPhysicalDevice> devices;
        uint32_t deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        devices.resize(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        if (deviceCount == 0) {
            throw std::runtime_error("no available devices\n");
        } // add logic to pick the most ideal physical device
        bool devFound = false;
        for (auto pdev : devices) {
            if (isDeviceSuitable(pdev)) {
                physicalDevice = pdev;
                devFound = true;
                break;
            }
        }
        if (!devFound) throw std::runtime_error("no suitable physical device\n");

        uint32_t queueFamilyCount;
        std::vector<VkQueueFamilyProperties> queueFamilyProps;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        queueFamilyProps.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());
        int idx = 0;
        bool hasGraphicsQueue = false;
        for (const auto &queueFamily : queueFamilyProps) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                hasGraphicsQueue = true;
                break;
            } 
            idx++;
        }
        if (!hasGraphicsQueue) throw std::runtime_error("no graphics queue available\n");
        queue.graphicsQueueIndex = idx;
        float priority = 1.0f;

        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.pQueuePriorities = &priority;
        queueInfo.queueCount = 1;
        queueInfo.queueFamilyIndex = queue.graphicsQueueIndex;

        VkPhysicalDeviceFeatures features{};
        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;

        std::vector<const char*> extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_2_EXTENSION_NAME
        };

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledExtensionCount = extensions.size();
        deviceInfo.ppEnabledExtensionNames = extensions.data();
        deviceInfo.enabledLayerCount = 0;
        deviceInfo.pEnabledFeatures = &features;
        deviceInfo.pNext = &features13;

        if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create device\n");
        } else {
            printf("device created\n");
        }

        vkGetDeviceQueue(device, queue.graphicsQueueIndex, 0, &queue.queue);
    }

    void createSurface() {
        VkResult res = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create surface\n");
        }
    }

    VkExtent2D chooseSwapChainExtent(VkSurfaceCapabilitiesKHR capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    void createSwapchain() { 
        VkSurfaceCapabilitiesKHR pSurfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &pSurfaceCapabilities);
        extent = chooseSwapChainExtent(pSurfaceCapabilities);
        format = VK_FORMAT_B8G8R8A8_SRGB;

        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.minImageCount = 3;
        swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainInfo.clipped = true;
        swapchainInfo.imageFormat = format;
        swapchainInfo.imageExtent = extent;
        swapchainInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.preTransform = pSurfaceCapabilities.currentTransform;
        swapchainInfo.surface = surface;

        if (vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swapchain\n");
        } else {
            printf("swapchain created\n");
        }
    }

    void createSwapchainImages() {
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = format;
        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        
        for (auto &image : swapchainImages) {
            VkImageView imageView;
            imageViewInfo.image = image;
            vkCreateImageView(device, &imageViewInfo, nullptr, &imageView);
            swapchainImageViews.push_back(imageView);
        }
    }

    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);\

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSize = {};
        poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
        descriptorPoolInfo.poolSizeCount = 1;
        descriptorPoolInfo.pPoolSizes = &poolSize;
        descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
    }

    void allocateDescriptorSets() {
        VkDescriptorSetLayoutBinding uboDescriptorLayoutBinding = {};
        uboDescriptorLayoutBinding.binding = 0;
        uboDescriptorLayoutBinding.descriptorCount = 1;
        uboDescriptorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboDescriptorLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {};
        descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutInfo.bindingCount = 1;
        descriptorLayoutInfo.pBindings = &uboDescriptorLayoutBinding;
        vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorLayout);

        for (int i = 0; i<MAX_FRAMES_IN_FLIGHT; i++) {
            layouts.push_back(descriptorLayout);        
        }

        uboBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uboMemories.resize(MAX_FRAMES_IN_FLIGHT);
        uboDatas.resize(MAX_FRAMES_IN_FLIGHT);
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorSetAllocateInfo allocateDescriptorsInfo = {};
        allocateDescriptorsInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocateDescriptorsInfo.descriptorPool = descriptorPool;
        allocateDescriptorsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateDescriptorsInfo.pSetLayouts = layouts.data();
        vkAllocateDescriptorSets(device, &allocateDescriptorsInfo, descriptorSets.data());

        ubo.model = glm::mat4(1.0f);
        ubo.view = lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 1000.0f);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,sizeof(UniformBufferObject), &uboBuffers[i], &uboMemories[i]);
            vkMapMemory(device, uboMemories[i], 0, sizeof(UniformBufferObject), 0, &uboDatas[i]);
            memcpy(uboDatas[i], &ubo, sizeof(ubo));
            // vkUnmapMemory(device, uboMemories[i]);

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = uboBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet writeDesc = {};
            writeDesc.descriptorCount = 1;
            writeDesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDesc.pBufferInfo = &bufferInfo;
            writeDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDesc.dstBinding = 0; 
            writeDesc.dstSet = descriptorSets[i]; 

            vkUpdateDescriptorSets(device, 1, &writeDesc, 0, nullptr);
        }


    }

    void createPipeline() {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        VkShaderModule vertShaderModule;
        VkShaderModuleCreateInfo vertShaderModuleInfo = {};
        vertShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertShaderModuleInfo.codeSize = vertShaderCode.size();
        vertShaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());
    
        if (vkCreateShaderModule(device, &vertShaderModuleInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module\n");
        }

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkShaderModule fragShaderModule;
        VkShaderModuleCreateInfo fragShaderModuleInfo = {};
        fragShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragShaderModuleInfo.codeSize = fragShaderCode.size();
        fragShaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());

        if (vkCreateShaderModule(device, &fragShaderModuleInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module\n");
        } 

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";
    
        VkPipelineShaderStageCreateInfo shaderStagesInfos[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        dynamicState.pDynamicStates = dynamicStates;
        
        VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &layouts[0];
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        } 

        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = sizeof(glm::vec3);

        std::vector<VkVertexInputAttributeDescription> attributeDescriptors;
        
        VkVertexInputAttributeDescription attributeDescriptionPos = {};
        attributeDescriptionPos.binding = 0;
        attributeDescriptionPos.location = 0;
        attributeDescriptionPos.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptionPos.offset = 0;

        // VkVertexInputAttributeDescription attributeDescriptionColor = {};
        // attributeDescriptionColor.binding = 0;
        // attributeDescriptionColor.location = 1;
        // attributeDescriptionColor.format = VK_FORMAT_R32G32B32_SFLOAT;
        // attributeDescriptionColor.offset = offsetof(Vertex, color);

        attributeDescriptors.push_back(attributeDescriptionPos);
        // attributeDescriptors.push_back(attributeDescriptionColor);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptors.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptors.size());
        
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
        rasterizationInfo.depthClampEnable = VK_FALSE;
        rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationInfo.depthBiasEnable = VK_FALSE;
        rasterizationInfo.lineWidth = 1.0f;
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
        multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingInfo.sampleShadingEnable = VK_FALSE;
        multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        VkPipelineColorBlendAttachmentState attachmentInfo = {};
        attachmentInfo.blendEnable = VK_FALSE;
        attachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; 
        VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
        colorBlendInfo.logicOpEnable = VK_FALSE;
        colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendInfo.attachmentCount = 1;
        colorBlendInfo.pAttachments = &attachmentInfo;

        VkPipelineViewportStateCreateInfo viewPortInfo = {};
        viewPortInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewPortInfo.scissorCount = 1;
        viewPortInfo.viewportCount = 1;

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStagesInfos;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pRasterizationState = &rasterizationInfo;
        pipelineInfo.pColorBlendState = &colorBlendInfo;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pMultisampleState = &multisamplingInfo;
        pipelineInfo.pViewportState = &viewPortInfo;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = nullptr;
        pipelineInfo.pNext = &pipelineRenderingCreateInfo;  

        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        printf("pipeline created successfully\n");
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDeviceMemoryProperties memPro) {
        for (uint32_t i = 0; i< memPro.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memPro.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
            }
        }  
        throw std::runtime_error("failed to find suitable memory type");

        return 0;
        }
    
    void createBuffer(VkBufferUsageFlags usageFlags, uint64_t size, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = usageFlags;
        bufferInfo.size = size;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bufferInfo, nullptr, buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer\n");
        }
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device, *buffer, &memoryRequirements);
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, propertyFlags, memoryProperties);

        if (vkAllocateMemory(device, &allocateInfo, nullptr, bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer\n");
        }
        vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
    }

    void createVertexAndIndexBuffers() {
        createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.size()*sizeof(vertices[0]), &vertexBuffer, &vertexBufferMemory);
        createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices.size()*sizeof(indices[0]), &indexBuffer, &indexBufferMemory);
        void * vertexData;
        void * indexData;
        vkMapMemory(device, vertexBufferMemory, 0, vertices.size()*sizeof(vertices[0]), 0, &vertexData);
        vkMapMemory(device, indexBufferMemory, 0, indices.size()*sizeof(indices[0]), 0, &indexData);
        memcpy(vertexData, vertices.data(), vertices.size()*sizeof(vertices[0]));
        memcpy(indexData, indices.data(), indices.size()*sizeof(indices[0]));
        vkUnmapMemory(device, vertexBufferMemory);
        vkUnmapMemory(device, indexBufferMemory);
    }

    void createSyncObjects() {
        presentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderedFinishedSemaphores.resize(swapchainImages.size());
        drawFences.resize(MAX_FRAMES_IN_FLIGHT);
        VkSemaphoreCreateInfo presentSemaphoreInfo = {};
        presentSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkSemaphoreCreateInfo renderedFinishedInfo = {};
        renderedFinishedInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        int idx = 0;
        while (idx < MAX_FRAMES_IN_FLIGHT) {
            vkCreateSemaphore(device, &presentSemaphoreInfo, nullptr, &presentCompleteSemaphores[idx]);
            vkCreateFence(device, &fenceInfo, nullptr, &drawFences[idx]);
            idx++;
        }
        idx = 0;
        while (idx < swapchainImages.size()) {
            vkCreateSemaphore(device, &renderedFinishedInfo, nullptr, &renderedFinishedSemaphores[idx]);
            idx++;
        }
    }

    void createCommandBufferPool() {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queue.graphicsQueueIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

    }

    void allocateCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if (vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffer");
        }
    }

    void transition_image_layout(VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags2 src_access_mask, VkAccessFlags2 dst_access_mask,  VkPipelineStageFlags2 src_stage_mask, VkPipelineStageFlags2 dst_stage_mask) 
    {
        VkImageMemoryBarrier2 barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = swapchainImages[imageIndex];
        barrier.srcAccessMask = src_access_mask;
        barrier.dstAccessMask = dst_access_mask;
        barrier.srcStageMask = src_stage_mask;
        barrier.dstStageMask = dst_stage_mask;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; 
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; 
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkDependencyInfo dependencyInfo = {};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(commandBuffers[currentFrame], &dependencyInfo);    
    }

    void recordCommandBuffer() {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        VkRenderingAttachmentInfo renderingAttachmentInfo = {};
        renderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        renderingAttachmentInfo.clearValue = clearColor;
        renderingAttachmentInfo.imageView = swapchainImageViews[imageIndex];
        renderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        renderingAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        renderingAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;

        VkRenderingInfo renderingInfo = {};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {.offset = {0, 0}, .extent = extent};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &renderingAttachmentInfo;

        if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin command buffer");
        }

        transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, {}, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        
        vkCmdBeginRendering(commandBuffers[currentFrame], &renderingInfo);

        vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
        
        vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

        VkDeviceSize offset = 0;

        vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, &vertexBuffer, &offset);

        vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,1, &descriptorSets[currentFrame], 0, nullptr);

        // vkCmdDraw(commandBuffers[currentFrame], vertices.size(), 1, 0, 0);

        vkCmdDrawIndexed(commandBuffers[currentFrame], indices.size(), 1, 0, 0, 0);

        vkCmdEndRendering(commandBuffers[currentFrame]);    

        transition_image_layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, {}, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

        if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void handleInput(float deltaTime) {
        float speed = 25.0f * deltaTime;
        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
        if (glfwGetKey(window, GLFW_KEY_W)) cameraPos += speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S)) cameraPos -= speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A)) cameraPos -= speed * right;
        if (glfwGetKey(window, GLFW_KEY_D)) cameraPos += speed * right;
        if (glfwGetKey(window, GLFW_KEY_E)) cameraPos += speed * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_F)) cameraPos -= speed * cameraUp;

    }

    void handleMouse() {
        
        double newX, newY;
        glfwGetCursorPos(window, &newX, &newY);
        double dx = newX - xpos, dy = ypos - newY; // deltas, not absolute
        xpos = newX; ypos = newY;

        float sensitivity = 0.1f;
        yaw   += dx * sensitivity;
        pitch -= dy * sensitivity;
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        glm::vec3 dir;
        dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        dir.y = sin(glm::radians(pitch));
        dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(dir);

        ubo.view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    }

    void mainLoop() {
        lastFrameTime = glfwGetTime();
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            
            double currentTime = glfwGetTime();
            deltaTime = static_cast<float>(currentTime - lastFrameTime);
            lastFrameTime = currentTime;

            handleInput(deltaTime);
            handleMouse();

            memcpy(uboDatas[currentFrame], &ubo, sizeof(UniformBufferObject));


            auto fenceResult = vkWaitForFences(device, 1, &drawFences[currentFrame], VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &drawFences[currentFrame]);
            auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentCompleteSemaphores[currentFrame], nullptr, &imageIndex);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to acquire next image\n");
            }

            vkResetCommandBuffer(commandBuffers[currentFrame], 0);
            recordCommandBuffer();

            VkPipelineStageFlags waitDestinationStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentFrame];
            submitInfo.pSignalSemaphores = &renderedFinishedSemaphores[imageIndex];
            submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
            VkResult v = vkQueueSubmit(queue.queue, 1, &submitInfo, drawFences[currentFrame]);

            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &renderedFinishedSemaphores[imageIndex];
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapchain;
            
            vkQueuePresentKHR(queue.queue, &presentInfo);

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }
    }
};

int main() {
    Core Engine;
    Engine.runApp();
}