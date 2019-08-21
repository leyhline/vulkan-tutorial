#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
const uint32_t validationLayersCount = 1;
const char *const validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};
const uint32_t deviceExtensionsCount = 1;
const char *const deviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const int WIDTH = 800;
const int HEIGHT = 600;

static void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_RELEASE) printf("Key pressed: %i\n", key);
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void initWindow(GLFWwindow **window) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    if (!glfwVulkanSupported()) {
        fprintf(stderr, "ERROR: Vulkan not supported\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    int major, minor, revision;
    glfwGetVersion(&major, &minor, &revision);
    printf("GLFW header version:  %u.%u.%u\n",
           GLFW_VERSION_MAJOR,
           GLFW_VERSION_MINOR,
           GLFW_VERSION_REVISION);
    printf("GLFW library version: %u.%u.%u\n", major, minor, revision);
    printf("GLFW library string:  %s\n", glfwGetVersionString());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
    *window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Playground", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(*window, key_callback);
}

void mainLoop(GLFWwindow *window) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void cleanUp(GLFWwindow **window, VkInstance *vulkanInstance, VkDevice *device, VkSurfaceKHR *surface) {
    vkDestroyDevice(*device, NULL);
    vkDestroySurfaceKHR(*vulkanInstance, *surface, NULL);
    vkDestroyInstance(*vulkanInstance, NULL);
    glfwDestroyWindow(*window);
    glfwTerminate();
}

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    VkLayerProperties *availableLayers;
    bool layerFound = false;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    availableLayers = (VkLayerProperties *) malloc(layerCount * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
    for (uint32_t j = 0; j < validationLayersCount; ++j) {
        for (uint32_t i = 0; i < layerCount; ++i) {
            if (strcmp(validationLayers[j], availableLayers[i].layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    free(availableLayers);
    return true;
}

void createInstance(VkInstance *instance) {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        fprintf(stderr, "ERROR Vulkan: validation layers requested but not available\n");
    }
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = validationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }
    if (vkCreateInstance(&createInfo, NULL, instance) != VK_SUCCESS) {
        fprintf(stderr, "Vulkan: failed to create instance\n");
    }
}

bool findGraphicsQueueFamilyIndex(VkPhysicalDevice device, VkSurfaceKHR *surface, uint32_t *graphicsFamilyIndex) {
    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties *queueFamilies;
    bool graphicsQueueSupported = false;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    queueFamilies = (VkQueueFamilyProperties *) malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, *surface, &presentSupport);
        if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport) {
            graphicsQueueSupported = true;
            *graphicsFamilyIndex = i;
            break;
        }
    }
    free(queueFamilies);
    return graphicsQueueSupported;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    bool requiredExtensionsSupported = true;
    uint32_t extensionCount;
    VkExtensionProperties *availableExtensions;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
    availableExtensions = (VkExtensionProperties *) malloc(extensionCount * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);
    for (uint32_t j = 0; j < deviceExtensionsCount; ++j) {
        bool currentExtensionSupported = false;
        for (uint32_t i = 0; i < extensionCount; ++i) {
            if (strcmp(deviceExtensions[j], availableExtensions[i].extensionName) == 0) {
                currentExtensionSupported = true;
                break;
            }
        }
        requiredExtensionsSupported = requiredExtensionsSupported && currentExtensionSupported;
    }
    free(availableExtensions);
    return requiredExtensionsSupported;
}

bool querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR *surface) {
    bool swapChainAdequate = false;
    VkSurfaceCapabilitiesKHR capabilities;
    uint32_t formatCount = 0;
    VkSurfaceFormatKHR *formats;
    uint32_t presentModeCount = 0;
    VkPresentModeKHR *presentModes;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, *surface, &capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, *surface, &formatCount, NULL);
    if (formatCount > 0) {
        formats = (VkSurfaceFormatKHR *) malloc(formatCount * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, *surface, &formatCount, formats);
    }
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, *surface, &presentModeCount, NULL);
    if (presentModeCount > 0) {
        presentModes = (VkPresentModeKHR *) malloc(presentModeCount * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, *surface, &presentModeCount, presentModes);
    }
    swapChainAdequate = (formatCount > 0) && (presentModeCount > 0);
    free(presentModes);
    free(formats);
    return swapChainAdequate;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR *surface) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    uint32_t graphicsFamilyIndex = 0;
    bool queueAdequate = findGraphicsQueueFamilyIndex(device, surface, &graphicsFamilyIndex);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        swapChainAdequate = querySwapChainSupport(device, surface);
    }
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    printf("Vulkan GPU name: %s\n", deviceProperties.deviceName);
    return queueAdequate && extensionsSupported && swapChainAdequate;
}

void pickPhysicalDevice(VkInstance *instance, VkSurfaceKHR *surface, VkPhysicalDevice *physicalDevice) {
    *physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    VkPhysicalDevice *devices;
    vkEnumeratePhysicalDevices(*instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        fprintf(stderr, "ERROR Vulkan: failed to find GPUs with Vulkan support\n");
    }
    devices = (VkPhysicalDevice *) malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(*instance, &deviceCount, devices);
    for (uint32_t i = 0; i < deviceCount; ++i) {
        if (isDeviceSuitable(devices[i], surface)) {
            *physicalDevice = devices[i];
            break;
        }
    }
    if (*physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "ERROR Vulkan: failed to find suitable GPU\n");
    }
    free(devices);
}

void createLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR *surface, VkDevice *logicalDevice, VkQueue *presentQueue) {
    uint32_t graphicsFamilyIndex = 0;
    if (!findGraphicsQueueFamilyIndex(physicalDevice, surface, &graphicsFamilyIndex)) {
        fprintf(stderr, "ERROR Vulkan: Could not find graphics queue\n");
    }
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayersCount;
        createInfo.ppEnabledLayerNames = validationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }
    createInfo.enabledExtensionCount = deviceExtensionsCount;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    if (vkCreateDevice(physicalDevice, &createInfo, NULL, logicalDevice) != VK_SUCCESS) {
        fprintf(stderr, "ERROR Vulkan: failed to create logical device\n");
    }
    vkGetDeviceQueue(*logicalDevice, graphicsFamilyIndex, 0, presentQueue);
}

void createSurface(VkInstance *instance, GLFWwindow **window, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(*instance, *window, NULL, surface) != VK_SUCCESS) {
        fprintf(stderr, "ERROR Vulkan: failed to create window surface\n");
    }
}

void initVulkan(VkInstance *instance, GLFWwindow **window, VkDevice *logicalDevice, VkSurfaceKHR *surface) {
    VkPhysicalDevice physicalDevice;
    VkQueue presentQueue;
    createInstance(instance);
    createSurface(instance, window, surface);
    pickPhysicalDevice(instance, surface, &physicalDevice);
    querySwapChainSupport(physicalDevice, surface);
    createLogicalDevice(physicalDevice, surface, logicalDevice, &presentQueue);
}

int main(const int argc, const char *argv[]) {
    GLFWwindow *window;
    VkInstance vulkanInstance;
    VkDevice device;
    VkSurfaceKHR surface;
    initWindow(&window);
    initVulkan(&vulkanInstance, &window, &device, &surface);
    mainLoop(window);
    cleanUp(&window, &vulkanInstance, &device, &surface);
    return 0;
}