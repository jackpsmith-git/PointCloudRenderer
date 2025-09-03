#include "DescriptorPool.h"

// PCR
#include "CommandBuffers.h"

// STD
#include <array>

DescriptorPool::DescriptorPool(std::shared_ptr<Device> device, std::shared_ptr<RenderPass> renderPass, std::shared_ptr<Swapchain> swapchain, std::shared_ptr<Buffer> triangleBuffer, std::shared_ptr<Buffer> pointBuffer)
	: m_device(device), m_renderPass(renderPass), m_swapchain(swapchain)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_device->GetGraphicsFamilyIndex();
    VkCommandPool commandPool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(m_device->Get(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool!");

    m_commandBuffers = std::make_shared<CommandBuffers>(m_device->Get(), commandPool,
        m_renderPass->Get(),
        m_renderPass->GetFramebuffers(),
        m_swapchain->GetExtent());

    VkDescriptorSetLayoutBinding triBinding{};
    triBinding.binding = 0;
    triBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    triBinding.descriptorCount = 1;
    triBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding pointBinding{};
    pointBinding.binding = 1;
    pointBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pointBinding.descriptorCount = 1;
    pointBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> descBindings = { triBinding, pointBinding };

    VkDescriptorSetLayoutCreateInfo descLayoutInfo{};
    descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descLayoutInfo.bindingCount = static_cast<uint32_t>(descBindings.size());
    descLayoutInfo.pBindings = descBindings.data();

    if (vkCreateDescriptorSetLayout(m_device->Get(), &descLayoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");

    VkDescriptorPoolSize poolSizes[1];
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = 2;

    VkDescriptorPoolCreateInfo poolCreate{};
    poolCreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreate.poolSizeCount = 1;
    poolCreate.pPoolSizes = poolSizes;
    poolCreate.maxSets = 1;

    VkDescriptorPool descPool;
    if (vkCreateDescriptorPool(m_device->Get(), &poolCreate, nullptr, &descPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = GetDescriptorSetLayout();

    if (vkAllocateDescriptorSets(m_device->Get(), &allocInfo, &m_computeDescriptorSet) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor set!");

    VkDescriptorBufferInfo triBufInfo{};
    triBufInfo.buffer = triangleBuffer->Get();
    triBufInfo.offset = 0;
    triBufInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo pointBufInfo{};
    pointBufInfo.buffer = pointBuffer->Get();
    pointBufInfo.offset = 0;
    pointBufInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet writeTri{};
    writeTri.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeTri.dstSet = m_computeDescriptorSet;
    writeTri.dstBinding = 0;
    writeTri.dstArrayElement = 0;
    writeTri.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeTri.descriptorCount = 1;
    writeTri.pBufferInfo = &triBufInfo;

    VkWriteDescriptorSet writePoint{};
    writePoint.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writePoint.dstSet = m_computeDescriptorSet;
    writePoint.dstBinding = 1;
    writePoint.dstArrayElement = 0;
    writePoint.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writePoint.descriptorCount = 1;
    writePoint.pBufferInfo = &pointBufInfo;

    std::array<VkWriteDescriptorSet, 2> writeSets = { writeTri, writePoint };
    vkUpdateDescriptorSets(m_device->Get(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

DescriptorPool::~DescriptorPool()
{
	vkDestroyDescriptorSetLayout(m_device->Get(), m_descriptorSetLayout, nullptr);
}
