#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

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

size_t readShaderFromFile(const char filename[], uint32_t **shaderContent) {
    FILE *fp;
    size_t filesize;
    char *buffer;
    int offset;
    size_t resultsize;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "ERROR opening file: %s\n", filename);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    offset = (4 - (filesize % 4)) % 4;
    rewind(fp);
    buffer = (char *) malloc((filesize + offset) * sizeof(char));
    resultsize = fread(buffer, 1, filesize, fp);
    if (resultsize != filesize) {
        fprintf(stderr, "ERROR reading file: %s\n", *filename);
    }
    memset(buffer + filesize, 0, offset);
    fclose(fp);
    *shaderContent = (uint32_t*) buffer;
    return filesize + offset;
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

void cleanUp(GLFWwindow **window, VkInstance *vulkanInstance, VkDevice *device, VkSurfaceKHR *surface, VkSwapchainKHR *swapchain, VkPipelineLayout *pipelineLayout, VkRenderPass *renderPass, VkPipeline *pipeline) {
    vkDestroyPipeline(*device, *pipeline, NULL);
    vkDestroyPipelineLayout(*device, *pipelineLayout, NULL);
    vkDestroyRenderPass(*device, *renderPass, NULL);
    vkDestroySwapchainKHR(*device, *swapchain, NULL);
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

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, uint32_t formatCount) {
    for (uint32_t i = 0; i < formatCount; ++i) {
        if (   availableFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM 
            && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats[i];
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR *availableModes, uint32_t presentModeCount) {
    for (uint32_t i = 0; i < presentModeCount; ++i) {
        if (availableModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availableModes[i];
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t uint32_min(uint32_t x, uint32_t y) {
    return x < y ? x : y;
}

uint32_t uint32_max(uint32_t x, uint32_t y) {
    return x < y ? y : x;
}

VkExtent2D chooseSwapExtend(VkSurfaceCapabilitiesKHR *capabilities) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    } else {
        VkExtent2D actualExtend;
        actualExtend.width = uint32_max(capabilities->minImageExtent.width, uint32_min(capabilities->maxImageExtent.width, WIDTH));
        actualExtend.height = uint32_max(capabilities->minImageExtent.height, uint32_min(capabilities->maxImageExtent.height, HEIGHT));
        return actualExtend;
    }
}

bool querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR *surface, VkSwapchainCreateInfoKHR *swapchainCreateInfo) {
    bool swapChainAdequate = false;
    VkSurfaceCapabilitiesKHR capabilities;
    VkExtent2D imageExtend;
    uint32_t imageCount = 0;
    uint32_t formatCount = 0;
    VkSurfaceFormatKHR *formats;
    VkSurfaceFormatKHR bestFormat;
    uint32_t presentModeCount = 0;
    VkPresentModeKHR *presentModes;
    VkPresentModeKHR bestPresentMode;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, *surface, &capabilities);
    imageExtend = chooseSwapExtend(&capabilities);
    imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, *surface, &formatCount, NULL);
    if (formatCount > 0) {
        formats = (VkSurfaceFormatKHR *) malloc(formatCount * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, *surface, &formatCount, formats);
        bestFormat = chooseSwapSurfaceFormat(formats, formatCount);
    }
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, *surface, &presentModeCount, NULL);
    if (presentModeCount > 0) {
        presentModes = (VkPresentModeKHR *) malloc(presentModeCount * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, *surface, &presentModeCount, presentModes);
        bestPresentMode = chooseSwapPresentMode(presentModes, presentModeCount);
    }
    swapChainAdequate = (formatCount > 0) && (presentModeCount > 0);
    if (swapchainCreateInfo != NULL) {
        swapchainCreateInfo->sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo->surface = *surface;
        swapchainCreateInfo->minImageCount = imageCount;
        swapchainCreateInfo->imageFormat = bestFormat.format;
        swapchainCreateInfo->imageColorSpace = bestFormat.colorSpace;
        swapchainCreateInfo->imageExtent = imageExtend;
        swapchainCreateInfo->imageArrayLayers = 1;
        swapchainCreateInfo->imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo->queueFamilyIndexCount = 0;
        swapchainCreateInfo->pQueueFamilyIndices = NULL;
        swapchainCreateInfo->preTransform = capabilities.currentTransform;
        swapchainCreateInfo->compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo->presentMode = bestPresentMode;
        swapchainCreateInfo->clipped = VK_TRUE;
        swapchainCreateInfo->oldSwapchain = VK_NULL_HANDLE;
    }
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
        swapChainAdequate = querySwapChainSupport(device, surface, NULL);
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

void createImageViews(VkDevice *device, VkImage *images, uint32_t imageCount, VkFormat imageFormat, VkImageView *imageViews) {
    imageViews = (VkImageView *) malloc(imageCount * sizeof(VkImageView));
    for (uint32_t i = 0; i < imageCount; ++i) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = imageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(*device, &createInfo, NULL, &imageViews[i]) != VK_SUCCESS) {
            fprintf(stderr, "ERROR Vulkan: failed to create image views");
        }
        vkDestroyImageView(*device, imageViews[i], NULL);
    }
    free(imageViews);
}

VkShaderModule createShaderModule(VkDevice device, const uint32_t *shaderContent, size_t shaderSize) {
    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    assert(shaderSize % 4 == 0);
    createInfo.codeSize = shaderSize;
    assert (shaderContent != NULL);
    createInfo.pCode = shaderContent;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        fprintf(stderr, "ERROR Vulkan: failed to create shader module");
    }
    return shaderModule;
}

void createGraphicsPipeline(VkDevice device, VkRenderPass *renderPass, VkExtent2D *imageExtent, VkPipelineLayout *pipelineLayout, VkPipeline *pipeline) {
    uint32_t *vertexShader;
    size_t vertexShaderSize;
    VkShaderModule vertexShaderModule;
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    uint32_t *fragmentShader;
    size_t fragmentShaderSize;
    VkShaderModule fragmentShaderModule;
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    vertexShaderSize = readShaderFromFile("shaders/vert.spv", &vertexShader);
    vertexShaderModule = createShaderModule(device, vertexShader, vertexShaderSize);
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertexShaderModule;
    vertShaderStageInfo.pName = "main";
    fragmentShaderSize = readShaderFromFile("shaders/frag.spv", &fragmentShader);
    fragmentShaderModule = createShaderModule(device, fragmentShader, fragmentShaderSize);
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragmentShaderModule;
    fragShaderStageInfo.pName = "main";
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = NULL;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = NULL;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) imageExtent->width;
    viewport.height = (float) imageExtent->height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = *imageExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = NULL;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = NULL;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = NULL;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "ERROR Vulkan: failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = NULL;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = NULL;
    pipelineInfo.layout = *pipelineLayout;
    pipelineInfo.renderPass = *renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline) != VK_SUCCESS) {
        fprintf(stderr, "ERROR Vulkan: failed to create graphics pipeline\n");
    }

    vkDestroyShaderModule(device, vertexShaderModule, NULL);
    vkDestroyShaderModule(device, fragmentShaderModule, NULL);
    free(vertexShader);
    free(fragmentShader);
}

void createRenderPass(VkDevice device, VkFormat imageFormat, VkRenderPass *renderPass) {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    if (vkCreateRenderPass(device, &renderPassInfo, NULL, renderPass) != VK_SUCCESS) {
        fprintf(stderr, "ERROR Vulkan: failed to create render pass\n");
    }
}

void initVulkan(VkInstance *instance, GLFWwindow **window, VkDevice *logicalDevice, VkSurfaceKHR *surface, VkSwapchainKHR *swapchain, VkPipelineLayout *pipelineLayout, VkRenderPass *renderPass, VkPipeline *pipeline) {
    VkPhysicalDevice physicalDevice;
    VkQueue presentQueue;
    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    createInstance(instance);
    createSurface(instance, window, surface);
    pickPhysicalDevice(instance, surface, &physicalDevice);
    createLogicalDevice(physicalDevice, surface, logicalDevice, &presentQueue);
    querySwapChainSupport(physicalDevice, surface, &swapchainCreateInfo);
    if (vkCreateSwapchainKHR(*logicalDevice, &swapchainCreateInfo, NULL, swapchain) != VK_SUCCESS) {
        fprintf(stderr, "ERROR Vulkan: failed to create swap chain\n");
    }
    createRenderPass(*logicalDevice, swapchainCreateInfo.imageFormat, renderPass);
    createGraphicsPipeline(*logicalDevice, renderPass, &swapchainCreateInfo.imageExtent, pipelineLayout, pipeline);
}

int main(const int argc, const char *argv[]) {
    GLFWwindow *window;
    VkInstance vulkanInstance;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    initWindow(&window);
    initVulkan(&vulkanInstance, &window, &device, &surface, &swapchain, &pipelineLayout, &renderPass, &pipeline);

    uint32_t imageCount = 0;
    VkImage *images;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL);
    images = (VkImage *) malloc(imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images);

    mainLoop(window);
    cleanUp(&window, &vulkanInstance, &device, &surface, &swapchain, &pipelineLayout, &renderPass, &pipeline);
    return 0;
}