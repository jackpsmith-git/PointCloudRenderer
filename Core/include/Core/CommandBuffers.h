#pragma once

// STD
#include <vector>

// Vulkan
#include <vulkan/vulkan.h>

class CommandBuffers
{
public:
	CommandBuffers(VkDevice device, VkCommandPool commandPool, VkRenderPass renderPass, const std::vector<VkFramebuffer>& framebuffers, VkExtent2D extent);
	~CommandBuffers() {};

	const std::vector<VkCommandBuffer>& GetCommandBuffers() const { return m_CommandBuffers; }

private:
	VkDevice m_Device;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	const std::vector<VkFramebuffer>& m_Framebuffers;
	VkExtent2D m_Extent;

	void RecordCommandBuffers(VkRenderPass renderPass);
};