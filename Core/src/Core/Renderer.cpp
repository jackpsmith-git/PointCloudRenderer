#include "Renderer.h"

// PCR
#include "CommandBuffers.h"
#include "PushConstants.h"
#include "Triangle.h"
#include "Utils.h"

// STD
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

// VULKAN
#include <vulkan/vulkan.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// TINYOBJ
#include "tinyobjloader/tiny_obj_loader.h"

// WIN32
#include <Windows.h>

Renderer::Renderer(std::shared_ptr<Window> window)
    : m_window(window)
{
    if (!window || window == nullptr)
    {
        Utils::ThrowFatalError("Invalid or null window pointer passed to renderer!");
    }

    m_instance = std::make_shared<Instance>(window.get());
    m_surface = m_instance->CreateVulkanSurface(window.get());

    m_device = std::make_shared<Device>(m_instance->Get(), m_surface);
    m_swapChain = std::make_shared<Swapchain>(m_device.get(), window.get());
    m_renderPass = std::make_shared<RenderPass>(m_device.get(), m_swapChain.get());
}

Renderer::~Renderer()
{
    if (m_surface != VK_NULL_HANDLE) 
    {
        vkDestroySurfaceKHR(m_instance->Get(), m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
}

void Renderer::LoadMesh(const char* modelPath)
{
	m_mesh = Mesh(modelPath);
	m_triangles = m_mesh.GetTriangles();
	m_meshLoaded = true;
}

void Renderer::Init()
{
    if (!m_meshLoaded)
    {
		Utils::ThrowFatalError("Mesh not loaded before initializing renderer!");
    }

    VkDevice device = m_device->Get();
    VkPhysicalDevice physicalDevice = m_device->GetPhysicalDevice();

    m_triangleBuffer = std::make_unique<Buffer>(
        device,
        physicalDevice,
        sizeof(Triangle) * m_triangles.size(),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    m_triangleBuffer->CopyData(m_triangles.data(), sizeof(Triangle) * m_triangles.size());

    m_particleBuffer = std::make_unique<Buffer>(
        device,
        physicalDevice,
        sizeof(glm::vec4) * m_particleCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    m_descriptorPool = std::make_shared<DescriptorPool>(m_device, m_renderPass, m_swapChain, m_triangleBuffer, m_particleBuffer);
    m_commandBuffers = m_descriptorPool->GetCommandBuffers();
	m_computePipeline = std::make_shared<ComputePipeline>(m_descriptorPool, m_device);
	m_graphicsPipeline = std::make_shared<GraphicsPipeline>(m_renderPass, m_swapChain, m_device);

    m_imageCount = static_cast<uint32_t>(m_swapChain->GetImageViews().size());
    m_imageAvailable.resize(m_imageCount);
    m_renderFinished.resize(m_imageCount);
    m_inFlightFences.resize(m_imageCount);
    m_imagesInFlight.resize(m_imageCount, VK_NULL_HANDLE);

    for (uint32_t i = 0; i < m_imageCount; ++i) 
    {
        VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        if (vkCreateSemaphore(m_device->Get(), &semInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS)
			Utils::ThrowFatalError("Failed to create imageAvailable semaphore.");
        if (vkCreateSemaphore(m_device->Get(), &semInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS)
            Utils::ThrowFatalError("Failed to create renderFinished semaphore.");

        VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (vkCreateFence(m_device->Get(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
            Utils::ThrowFatalError("Failed to create inFlight fence.");
    }
}

void Renderer::Run()
{
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device->Get(), m_swapChain->Get(), UINT64_MAX,
        m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) 
    {
        m_window->FramebufferResized = false;
        RecreateSwapchain();
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
    {
        Utils::ThrowFatalError("Failed to acquire swapchain image.");
    }

    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_device->Get(), 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

    vkWaitForFences(m_device->Get(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_device->Get(), 1, &m_inFlightFences[m_currentFrame]);

    VkCommandBuffer cmd = m_commandBuffers->Get()[imageIndex];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
        Utils::ThrowFatalError("Failed to begin command buffer.");

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline->Get());
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline->GetLayout(), 0, 1, m_descriptorPool->GetComputeDescriptorSet(), 0, nullptr);

    ComputePushConstants compPC{};
    compPC.time = static_cast<float>(clock() / static_cast<double>(CLOCKS_PER_SEC));
    compPC.numTriangles = static_cast<uint32_t>(m_triangles.size());
    vkCmdPushConstants(cmd, m_computePipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &compPC);

    vkCmdDispatch(cmd, (m_particleCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE, 1, 1);

    VkMemoryBarrier memBarrier{};
    memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        0,
        1, &memBarrier,
        0, nullptr,
        0, nullptr);

    m_renderPass->Begin(cmd, imageIndex);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->Get());

    VkDeviceSize offsets[] = { 0 };
    VkBuffer vb = m_particleBuffer->Get();
    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, offsets);

    GraphicsPushConstants gfxPC{};
    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
        (float)m_swapChain->GetExtent().width / (float)m_swapChain->GetExtent().height,
        0.1f, 100.0f);

    float angle = m_rotationSpeed * static_cast<float>(clock()) / CLOCKS_PER_SEC;
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

    proj[1][1] *= -1.0f;

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, m_window->CameraDistance), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    gfxPC.mvp = proj * view * model;
    vkCmdPushConstants(cmd, m_graphicsPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GraphicsPushConstants), &gfxPC);

    vkCmdDraw(cmd, m_particleCount, 1, 0, 0);

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        Utils::ThrowFatalError("Failed to record command buffer.");

    VkSemaphore waitSemaphores[] = { m_imageAvailable[m_currentFrame] };
    VkSemaphore signalSemaphores[] = { m_renderFinished[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
        Utils::ThrowFatalError("Failed to submit draw command buffer.");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR scs[] = { m_swapChain->Get() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = scs;
    presentInfo.pImageIndices = &imageIndex;

    VkResult pres = vkQueuePresentKHR(m_device->GetPresentQueue(), &presentInfo);
    if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR || m_window->FramebufferResized) 
    {
        m_window->FramebufferResized = false;
        RecreateSwapchain();
    }
    else if (pres != VK_SUCCESS) 
    {
        Utils::ThrowFatalError("Failed to present swapchain image.");
    }

    m_currentFrame = (m_currentFrame + 1) % m_imageCount;
}

void Renderer::Shutdown()
{
    vkDeviceWaitIdle(m_device->Get());

    for (uint32_t i = 0; i < m_imageCount; i++) {
        vkDestroySemaphore(m_device->Get(), m_imageAvailable[i], nullptr);
        vkDestroySemaphore(m_device->Get(), m_renderFinished[i], nullptr);
        vkDestroyFence(m_device->Get(), m_inFlightFences[i], nullptr);
    }
}

void Renderer::RecreateSwapchain()
{
    vkDeviceWaitIdle(m_device->Get());

    m_imagesInFlight.clear();
    m_imagesInFlight.resize(m_imageCount, VK_NULL_HANDLE);

    for (uint32_t i = 0; i < m_inFlightFences.size(); ++i)
    {
        if (m_imageAvailable[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device->Get(), m_imageAvailable[i], nullptr);
        if (m_renderFinished[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device->Get(), m_renderFinished[i], nullptr);
        if (m_inFlightFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(m_device->Get(), m_inFlightFences[i], nullptr);
    }

    m_graphicsPipeline.reset();
    m_renderPass.reset();
    m_swapChain.reset();
    m_commandBuffers.reset();
    m_descriptorPool.reset();

    m_swapChain = std::make_shared<Swapchain>(m_device.get(), m_window.get());
    m_renderPass = std::make_shared<RenderPass>(m_device.get(), m_swapChain.get());
    m_descriptorPool = std::make_shared<DescriptorPool>(m_device, m_renderPass, m_swapChain, m_triangleBuffer, m_particleBuffer);
    m_commandBuffers = m_descriptorPool->GetCommandBuffers();
    m_computePipeline = std::make_shared<ComputePipeline>(m_descriptorPool, m_device);
    m_graphicsPipeline = std::make_shared<GraphicsPipeline>(m_renderPass, m_swapChain, m_device);

    m_imageCount = static_cast<uint32_t>(m_swapChain->GetImageViews().size());
    m_imageAvailable.resize(m_imageCount);
    m_renderFinished.resize(m_imageCount);
    m_inFlightFences.resize(m_imageCount);

    VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < m_imageCount; ++i)
    {
        if (vkCreateSemaphore(m_device->Get(), &semInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS)
            Utils::ThrowFatalError("Failed to create imageAvailable semaphore");
        if (vkCreateSemaphore(m_device->Get(), &semInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS)
            Utils::ThrowFatalError("Failed to create renderFinished semaphore");
        if (vkCreateFence(m_device->Get(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
            Utils::ThrowFatalError("Failed to create inFlight fence");
    }

    m_imagesInFlight.resize(m_imageCount, VK_NULL_HANDLE);
}
