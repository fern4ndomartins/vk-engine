#include "../include/core.h"
#include <cstdio>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

class Core {
    public:
    GLFWwindow* window;
    const int WIDTH = 900; 
    const int HEIGHT = 500; 

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    GraphicsQueue queue;

    void runApp() {
        initVulkan();
    }
    void initVulkan() {
        createWindow();
        createInstance();
        createDevice();
    }
    void createWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, "vk engine", nullptr, nullptr);
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
    void createDevice() {
        std::vector<VkPhysicalDevice> devices;
        uint32_t deviceCount;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        devices.resize(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        if (deviceCount == 0) {
            throw std::runtime_error("no available devices\n");
        } // add logic to pick the most ideal physical device
        physicalDevice = devices[0];

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
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
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
};

int main() {
    Core Engine;
    Engine.runApp();
}