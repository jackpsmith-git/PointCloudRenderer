#pragma once

#include "Buffer.h"
#include "ComputePipeline.h"
#include "GraphicsPipeline.h"
#include "DescriptorPool.h"
#include "Device.h"
#include "Instance.h"
#include "Mesh.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include "Window.h"

class Renderer
{
public:
    Renderer();
	~Renderer();

	void LoadMesh(const char* modelPath);

    void Init(std::shared_ptr<Window> window);
	void Run();
    void Shutdown();

	void SetParticleCount(uint32_t count) { m_particleCount = count; }
	void SetRotationSpeed(float speed) { m_rotationSpeed = glm::radians(speed); }
private:
    void RecreateSwapchain();

private:
	std::shared_ptr<Window> m_window;
    std::shared_ptr<Instance> m_instance;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    std::shared_ptr<Device> m_device;
    std::shared_ptr<Swapchain> m_swapChain;
    std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<CommandBuffers> m_commandBuffers;

	std::shared_ptr<DescriptorPool> m_descriptorPool;
	std::shared_ptr<ComputePipeline> m_computePipeline;
	std::shared_ptr<GraphicsPipeline> m_graphicsPipeline;

    std::shared_ptr<Buffer> m_triangleBuffer = nullptr;
    std::shared_ptr<Buffer> m_particleBuffer = nullptr;

    uint32_t m_imageCount = 0;
    std::vector<VkSemaphore> m_imageAvailable;
	std::vector<VkSemaphore> m_renderFinished;
	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imagesInFlight;

	uint32_t m_currentFrame = 0;

    bool m_meshLoaded = false;
    Mesh m_mesh;
	std::vector<Triangle> m_triangles;

    const uint32_t WORK_GROUP_SIZE = 256;
    const uint32_t NUM_GROUPS = (m_particleCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE;

    uint32_t m_particleCount = 10000;
    float m_rotationSpeed = glm::radians(10.0f);
};