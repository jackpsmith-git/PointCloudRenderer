#include "Instance.h"

// PCR
#include "Utils.h"

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

Instance::Instance(Window* window, bool enableValidation)
{
	SDL_Window* sdlWindow = window->Get();

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Particle Mesh App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    unsigned int sdlExtensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(sdlWindow, &sdlExtensionCount, nullptr)) {
        Utils::ThrowFatalError("Failed to get SDL Vulkan extensions count");
    }

    std::vector<const char*> extensions(sdlExtensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(sdlWindow, &sdlExtensionCount, extensions.data())) {
        Utils::ThrowFatalError("Failed to get SDL Vulkan extensions names");
    }

    if (enableValidation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    std::vector<const char*> layers;
    if (enableValidation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        Utils::ThrowFatalError("Failed to create Vulkan instance!");
    }

    if (enableValidation) {
        SetupDebugMessenger();
    }
}

Instance::~Instance()
{
    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto destroyFn = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyFn) {
            destroyFn(m_instance, m_debugMessenger, nullptr);
        }
    }
    vkDestroyInstance(m_instance, nullptr);
}

VkSurfaceKHR Instance::CreateVulkanSurface(Window* window) const
{
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(window->Get(), m_instance, &surface))
    {
        Utils::ThrowFatalError("Failed to create Vulkan surface with SDL");
    }

    return surface;
}

void Instance::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = DebugCallback;

    auto createFn = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (createFn && createFn(m_instance, &info, nullptr, &m_debugMessenger) != VK_SUCCESS) {
        Utils::ThrowFatalError("Failed to set up debug messenger!");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Instance::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
    std::cerr << "[Vulkan] " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}
