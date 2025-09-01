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

	void Begin(VkCommandBuffer cmd, uint32_t imageIndex);
	
	VkRenderPass GetRenderPass() const { return m_renderPass; }
	const std::vector<VkFramebuffer>& GetFramebuffers() const { return m_Framebuffers; }
private:
	VkDevice m_Device;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_Framebuffers;
	VkExtent2D m_Extent;
	Swapchain* m_swapchain;

	void CreateRenderPass(VkFormat swapchainFormat);
	void CreateFramebuffers(const std::vector<VkImageView>& imageViews);
};