#pragma once

// PCR
#include "DescriptorPool.h"
#include "Device.h"
#include "RenderPass.h"
#include "Swapchain.h"

class GraphicsPipeline
{
public:
	GraphicsPipeline(std::shared_ptr<RenderPass> renderPass, std::shared_ptr<Swapchain> swapchain, std::shared_ptr<Device> device);
	~GraphicsPipeline();

	VkPipeline Get() const { return m_pipeline; }
	VkPipelineLayout GetLayout() const { return m_layout; }
private:
	std::shared_ptr<Device> m_device;

	VkPipelineLayout m_layout;
	VkPipeline m_pipeline;
};