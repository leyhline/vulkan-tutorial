#include <stdlib.h>
#include <stdio.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
        fprintf(stderr, "ERROR: Vulkan not supported");
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

void createInstance(VkInstance *instance) {
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
    createInfo.enabledLayerCount = 0;
    if (vkCreateInstance(&createInfo, NULL, instance) != VK_SUCCESS) {
        fprintf(stderr, "Vulkan: failed to create instance\n");
    }
}

void initVulkan(VkInstance *instance) {
    createInstance(instance);
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