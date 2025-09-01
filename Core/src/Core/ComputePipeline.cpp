#include "ComputePipeline.h"

// CORE
#include "PushConstants.h"
#include "DescriptorPool.h"
#include "Device.h"

// CORE
#include "Utils.h"

// STD
#include <stdexcept>

// VULKAN
#include <vulkan/vulkan.h>

ComputePipeline::ComputePipeline(std::shared_ptr<DescriptorPool> descriptorPool, std::shared_ptr<Device> device)
	: m_device(device)
{
    // --- Compute pipeline layout (with push constants for time + numTriangles) ---
    VkPushConstantRange compPush{};
    compPush.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    compPush.offset = 0;
    compPush.size = sizeof(ComputePushConstants);

    VkPipelineLayoutCreateInfo compLayoutInfo{};
    compLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    compLayoutInfo.setLayoutCount = 1;
    compLayoutInfo.pSetLayouts = descriptorPool->GetDescriptorSetLayout();
    compLayoutInfo.pushConstantRangeCount = 1;
    compLayoutInfo.pPushConstantRanges = &compPush;

    if (vkCreatePipelineLayout(device->GetDevice(), &compLayoutInfo, nullptr, &m_layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline layout!");

    auto compCode = Utils::ReadFile("shaders/pointcloud.comp.spv");
    VkShaderModule compShader = Utils::CreateShaderModule(device->GetDevice(), compCode);

    VkPipelineShaderStageCreateInfo compStage{};
    compStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compStage.module = compShader;
    compStage.pName = "main";

    VkComputePipelineCreateInfo computePipeInfo{};
    computePipeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipeInfo.stage = compStage;
    computePipeInfo.layout = m_layout;

    if (vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &computePipeInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline!");

    vkDestroyShaderModule(device->GetDevice(), compShader, nullptr);
}

ComputePipeline::~ComputePipeline()
{
    vkDestroyPipeline(m_device->GetDevice(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_device->GetDevice(), m_layout, nullptr);
}