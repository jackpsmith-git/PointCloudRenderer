#pragma once

// STD
#include <stdexcept>
#include <set>
#include <string>
#include <vector>

// VULKAN
#include <vulkan/vulkan.h>

class Device
{
public:
	Device(VkInstance instance, VkSurfaceKHR surface);
	~Device();

	VkDevice Get() const { return m_device; }
	VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
	VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
	VkQueue GetPresentQueue() const { return m_presentQueue; }
	VkSurfaceKHR GetSurface() const { return m_surface; }

    uint32_t GetGraphicsFamilyIndex() const { return m_graphicsFamily; }
    uint32_t GetPresentFamilyIndex() const { return m_presentFamily; }

private:

    struct QueueFamilyIndices 
    {
        int graphicsFamily = -1;
        int presentFamily = -1;
        bool IsComplete() const { return graphicsFamily >= 0 && presentFamily >= 0; }
    };

    void PickPhysicalDevice();
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	void CreateLogicalDevice();

private:
    VkInstance m_instance;
    VkSurfaceKHR m_surface;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    uint32_t m_graphicsFamily = -1;
    uint32_t m_presentFamily = -1;
};