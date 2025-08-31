#pragma once

// STD
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

// Vulkan
#include <vulkan/vulkan.h>

class Instance
{
public:
	Instance(SDL_Window* window, bool enableValidation = true);
	~Instance();

	VkInstance Get() const { return m_Instance; }

	VkSurfaceKHR CreateVulkanSurface(SDL_Window* window) const;
private:
	VkInstance m_Instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

	void SetupDebugMessenger();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void*);
};