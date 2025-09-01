#pragma once

#include "Window.h"

// STD
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

// Vulkan
#include <vulkan/vulkan.h>

class Instance
{
public:
	Instance(Window* window, bool enableValidation = true);
	~Instance();

	VkInstance Get() const { return m_Instance; }

	VkSurfaceKHR CreateVulkanSurface(Window* window) const;
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