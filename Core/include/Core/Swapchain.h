#pragma once

// PCR
#include "Device.h"
#include "Window.h"

// STD
#include <vector>

// VULKAN
#include <vulkan/vulkan.h>

class Swapchain
{
public:
	Swapchain(Device* device, Window* window);
	~Swapchain();

	VkSwapchainKHR Get() const { return m_swapchain; }
	const std::vector<VkImageView>& GetImageViews() const { return m_imageViews; }
	VkFormat GetFormat() const { return m_format; }
	VkExtent2D GetExtent() const { return m_extent; }

private:
    struct SwapchainSupportDetails 
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availableModes);
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
    void CreateSwapchain(uint32_t width, uint32_t height, uint32_t graphicsFamily, uint32_t presentFamily);
    void CreateImageViews();

private:
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_format;
    VkExtent2D m_extent;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
};