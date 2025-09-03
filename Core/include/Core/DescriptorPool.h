#pragma once

// PCR
#include "Buffer.h"
#include "CommandBuffers.h"
#include "Device.h"
#include "RenderPass.h"
#include "Swapchain.h"

// STD
#include <memory>

// VULKAN
#include <vulkan/vulkan.h>

class DescriptorPool
{
public:
	DescriptorPool(std::shared_ptr<Device> device, std::shared_ptr<RenderPass> renderPass, std::shared_ptr<Swapchain> swapchain, std::shared_ptr<Buffer> triangleBuffer, std::shared_ptr<Buffer> pointBuffer);
	~DescriptorPool();

	std::shared_ptr<CommandBuffers> GetCommandBuffers() const { return m_commandBuffers; }
	VkDescriptorSetLayout* GetDescriptorSetLayout() { return &m_descriptorSetLayout; }
	VkDescriptorSet* GetComputeDescriptorSet() { return &m_computeDescriptorSet; }
	
private:
	std::shared_ptr<Device> m_device;
	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Swapchain> m_swapchain;
	std::shared_ptr<CommandBuffers> m_commandBuffers;

	VkDescriptorSetLayout m_descriptorSetLayout;
	VkDescriptorSet m_computeDescriptorSet;
	VkDescriptorPool m_descriptorPool;
	VkCommandPool m_commandPool;

};