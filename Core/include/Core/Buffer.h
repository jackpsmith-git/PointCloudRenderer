#pragma once

// CORE
#include "Device.h"
#include "Triangle.h"

// VULKAN
#include <vulkan/vulkan.h>

class Buffer
{
public:
    Buffer(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    Buffer() {};

    ~Buffer();

    VkBuffer Get() const { return m_buffer; }
    VkDeviceMemory GetMemory() const { return m_memory; }
    VkDeviceSize GetSize() const { return m_size; }

    void* Map() const;
    void Unmap();
    void CopyData(const void* data, VkDeviceSize size);

private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;

    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

};