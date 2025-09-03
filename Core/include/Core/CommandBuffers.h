#pragma once

// STD
#include <vector>

// VULKAN
#include <vulkan/vulkan.h>

class CommandBuffers
{
public:
	CommandBuffers(VkDevice device, VkCommandPool commandPool, VkRenderPass renderPass, const std::vector<VkFramebuffer>& framebuffers, VkExtent2D extent);
	~CommandBuffers() {};

	const std::vector<VkCommandBuffer>& Get() const { return m_commandBuffers; }

private:
	VkDevice m_device;
	std::vector<VkCommandBuffer> m_commandBuffers;
	const std::vector<VkFramebuffer>& m_framebuffers;
	VkExtent2D m_extent;
};