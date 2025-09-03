#pragma once

// PCR
#include "Window.h"

// STD
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// VULKAN
#include <vulkan/vulkan.h>

class Instance
{
public:
	Instance(Window* window, bool enableValidation = true);
	~Instance();

	VkInstance Get() const { return m_instance; }

	VkSurfaceKHR CreateVulkanSurface(Window* window) const;

private:
	void SetupDebugMessenger();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void*);

private:
	VkInstance m_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
};