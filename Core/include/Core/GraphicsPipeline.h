#pragma once

// CORE
#include "DescriptorPool.h"
#include "Device.h"
#include "RenderPass.h"
#include "Swapchain.h"

class GraphicsPipeline
{
public:
	GraphicsPipeline(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<Swapchain> swapchain, std::shared_ptr<Device> device);
	~GraphicsPipeline();

	VkPipelineLayout GetLayout() const { return m_layout; }
	VkPipeline GetPipeline() const { return m_pipeline; }
private:
	std::shared_ptr<Device> m_device;

	VkPipelineLayout m_layout;
	VkPipeline m_pipeline;
};