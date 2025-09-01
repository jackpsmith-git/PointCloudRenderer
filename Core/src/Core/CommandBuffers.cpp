#include "CommandBuffers.h"

// STD
#include <stdexcept>

CommandBuffers::CommandBuffers(VkDevice device, VkCommandPool commandPool, VkRenderPass renderPass, const std::vector<VkFramebuffer>& framebuffers, VkExtent2D extent)
	: m_Device(device), m_Framebuffers(framebuffers), m_Extent(extent)
{
    m_CommandBuffers.resize(framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}
