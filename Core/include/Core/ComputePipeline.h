#pragma once

// CORE
#include "DescriptorPool.h"

class ComputePipeline
{
public:
	ComputePipeline(std::shared_ptr<DescriptorPool> descriptorPool, std::shared_ptr<Device> device);
	~ComputePipeline();

	VkPipelineLayout GetLayout() const { return m_layout; }
	VkPipeline GetPipeline() const { return m_pipeline; }

private:
	std::shared_ptr<Device> m_device;

	VkPipelineLayout m_layout;
	VkPipeline m_pipeline;
};