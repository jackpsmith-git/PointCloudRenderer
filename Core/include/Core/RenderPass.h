#pragma once

#include "Device.h"
#include "Swapchain.h"

// STD
#include <vector>

// Vulkan
#include <vulkan/vulkan.h>

class RenderPass
{
public: 
	RenderPass(Device* device, Swapchain* swapchain);
	~RenderPass();
	
	VkRenderPass GetRenderPass() const { return m_RenderPass; }
	const std::vector<VkFramebuffer>& GetFramebuffers() const { return m_Framebuffers; }
private:
	VkDevice m_Device;
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_Framebuffers;
	VkExtent2D m_Extent;

	void CreateRenderPass(VkFormat swapchainFormat);
	void CreateFramebuffers(const std::vector<VkImageView>& imageViews);
};