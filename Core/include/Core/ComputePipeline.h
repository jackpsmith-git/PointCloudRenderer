#pragma once

// PCR
#include "DescriptorPool.h"

class ComputePipeline
{
public:
	ComputePipeline(std::shared_ptr<DescriptorPool> descriptorPool, std::shared_ptr<Device> device);
	~ComputePipeline();

	VkPipeline Get() const { return m_pipeline; }
	VkPipelineLayout GetLayout() const { return m_layout; }

private:
	std::shared_ptr<Device> m_device;

	VkPipelineLayout m_layout;
	VkPipeline m_pipeline;
};