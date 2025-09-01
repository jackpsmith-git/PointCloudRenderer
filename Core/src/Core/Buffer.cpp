#include "Buffer.h"

// STD
#include <stdexcept>

Buffer::Buffer(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
    : m_device(device), m_physicalDevice(physicalDevice), m_size(size)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, m_buffer, m_memory, 0);
}

Buffer::~Buffer() 
{
    if (m_buffer != VK_NULL_HANDLE) 
    {
        vkDestroyBuffer(m_device, m_buffer, nullptr);
    }

    if (m_memory != VK_NULL_HANDLE) 
    {
        vkFreeMemory(m_device, m_memory, nullptr);
    }
}

void* Buffer::Map() const 
{
    void* data;
    vkMapMemory(m_device, m_memory, 0, m_size, 0, &data);
    return data;
}

void Buffer::Unmap() 
{
    vkUnmapMemory(m_device, m_memory);
}

void Buffer::CopyData(const void* data, VkDeviceSize size) 
{
    void* dst = Map();
    memcpy(dst, data, static_cast<size_t>(size));
    Unmap();
}

uint32_t Buffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const 
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}