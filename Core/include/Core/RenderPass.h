#pragma once

// PCR
#include "Device.h"
#include "Swapchain.h"

// STD
#include <vector>

// VULKAN
#include <vulkan/vulkan.h>

class RenderPass
{
public: 
	RenderPass(Device* device, Swapchain* swapchain);
	~RenderPass();

	void Begin(VkCommandBuffer cmd, uint32_t imageIndex);
	
	VkRenderPass Get() const { return m_renderPass; }
	const std::vector<VkFramebuffer>& GetFramebuffers() const { return m_framebuffers; }

private:
	void CreateRenderPass(VkFormat swapchainFormat);
	void CreateFramebuffers(const std::vector<VkImageView>& imageViews);

private:
	VkDevice m_device;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_framebuffers;
	VkExtent2D m_extent;
	Swapchain* m_swapchain;
};