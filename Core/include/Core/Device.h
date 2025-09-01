#pragma once

// STD
#include <vector>
#include <stdexcept>
#include <string>
#include <set>

// Vulkan
#include <vulkan/vulkan.h>

class Device
{
public:
	Device(VkInstance instance, VkSurfaceKHR surface);
	~Device();

	VkDevice GetDevice() const { return m_Device; }
	VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
	VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
	VkQueue GetPresentQueue() const { return m_PresentQueue; }
	VkSurfaceKHR GetSurface() const { return m_Surface; }

    uint32_t GetGraphicsFamilyIndex() const { return m_GraphicsFamily; }
    uint32_t GetPresentFamilyIndex() const { return m_PresentFamily; }

private:
    VkInstance m_Instance;
    VkSurfaceKHR m_Surface;

    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;

    uint32_t m_GraphicsFamily = -1;
    uint32_t m_PresentFamily = -1;

    struct QueueFamilyIndices {
        int graphicsFamily = -1;
        int presentFamily = -1;
        bool isComplete() { return graphicsFamily >= 0 && presentFamily >= 0; }
    };

    void PickPhysicalDevice();
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	void CreateLogicalDevice();
};