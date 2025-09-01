#pragma once

#include "Device.h"
#include "Window.h"

// Vulkan
#include <vulkan/vulkan.h>

// STD
#include <vector>

class Swapchain
{
public:
	Swapchain(Device* device, Window* window);
	~Swapchain();

	VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
	const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
	VkFormat GetFormat() const { return m_Format; }
	VkExtent2D GetExtent() const { return m_Extent; }

private:
    VkPhysicalDevice m_PhysicalDevice;
    VkDevice m_Device;
    VkSurfaceKHR m_Surface;

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkFormat m_Format;
    VkExtent2D m_Extent;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;

    struct SwapchainSupportDetails {
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
};