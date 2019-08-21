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
const size_t validationLayersCount = 1;
const char *const validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
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
    //glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
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

void cleanUp(GLFWwindow **window, VkInstance *vulkanInstance) {
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
    for (size_t j = 0; j < validationLayersCount; ++j) {
        for (size_t i = 0; i < layerCount; ++i) {
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
    appInfo.pNext = NULL;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = NULL;
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

bool findQueueFamilies(VkPhysicalDevice *device) {
    uint32_t queueFamilyCount = 0;
    VkQueueFamilyProperties *queueFamilies;
    bool graphicsQueueSupported = false;
    vkGetPhysicalDeviceQueueFamilyProperties(*device, &queueFamilyCount, NULL);
    queueFamilies = (VkQueueFamilyProperties *) malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(*device, &queueFamilyCount, queueFamilies);
    for (size_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueSupported = true;
            break;
        }
    }
    free(queueFamilies);
    return graphicsQueueSupported;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    printf("Vulkan GPU name: %s\n", deviceProperties.deviceName);
    return findQueueFamilies(&device);
}

void pickPhysicalDevice(VkInstance *instance, VkPhysicalDevice *physicalDevice) {
    *physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    VkPhysicalDevice *devices;
    vkEnumeratePhysicalDevices(*instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        fprintf(stderr, "ERROR Vulkan: failed to find GPUs with Vulkan support\n");
    }
    devices = (VkPhysicalDevice *) malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(*instance, &deviceCount, devices);
    for (size_t i = 0; i < deviceCount; ++i) {
        if (isDeviceSuitable(devices[i])) {
            *physicalDevice = devices[i];
            break;
        }
    }
    if (*physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "ERROR Vulkan: failed to find suitable GPU\n");
    }
    free(devices);
}

void initVulkan(VkInstance *instance) {
    VkPhysicalDevice device;
    createInstance(instance);
    pickPhysicalDevice(instance, &device);
}

int main(const int argc, const char *argv[]) {
    GLFWwindow *window;
    VkInstance vulkanInstance;
    initWindow(&window);
    initVulkan(&vulkanInstance);
    mainLoop(window);
    cleanUp(&window, &vulkanInstance);
    return 0;
}