#include "Renderer.h"

// CORE
#include "Buffer.h"
#include "CommandBuffers.h"
#include "EntryPoint.h"
#include "PushConstants.h"
#include "Triangle.h"
#include "Utils.h"

// STD
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <array>
#include <iostream>

// VULKAN
#include <vulkan/vulkan.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// TINYOBJ
#include "tinyobjloader/tiny_obj_loader.h"

Renderer::Renderer(const char* modelPath, uint32_t particleCount, float rotationSpeed)
	: m_mesh(modelPath), m_triangles(m_mesh.GetTriangles()), m_particleCount(particleCount), m_rotationSpeed(glm::radians(rotationSpeed))
{
    m_window = std::make_shared<Window>("Point-Cloud Renderer", 1280, 720);
    m_instance = std::make_shared<Instance>(m_window.get());
    m_surface = m_instance->CreateVulkanSurface(m_window.get());
    m_device = std::make_shared<Device>(m_instance->Get(), m_surface);
    m_swapChain = std::make_shared<Swapchain>(m_device.get(), m_window.get());
    m_renderPass = std::make_shared<RenderPass>(m_device.get(), m_swapChain.get());
}

Renderer::~Renderer()
{
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance->Get(), m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
}

void Renderer::Init()
{
    // Vulkan handles
    VkDevice deviceHandle = m_device->GetDevice();
    VkPhysicalDevice physical = m_device->GetPhysicalDevice();

    // --- Create buffers for triangles (SSBO) and points (SSBO + VERTEX) ---
    m_triangleBuffer = std::make_unique<Buffer>(
        deviceHandle,
        physical,
        sizeof(Triangle) * m_triangles.size(),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    m_triangleBuffer->CopyData(m_triangles.data(), sizeof(Triangle) * m_triangles.size());

    m_particleBuffer = std::make_unique<Buffer>(
        deviceHandle,
        physical,
        sizeof(glm::vec4) * m_particleCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    m_descriptorPool = std::make_shared<DescriptorPool>(m_device, m_renderPass, m_swapChain, m_triangleBuffer, m_particleBuffer);
    m_commandBuffers = m_descriptorPool->GetCommandBuffers();
	m_computePipeline = std::make_shared<ComputePipeline>(m_descriptorPool, m_device);
	m_graphicsPipeline = std::make_shared<GraphicsPipeline>(m_renderPass, m_swapChain, m_device);

    // --- Synchronization objects (per-frame) ---
    m_imageCount = static_cast<uint32_t>(m_swapChain->GetImageViews().size());
    m_imageAvailable.resize(m_imageCount);
    m_renderFinished.resize(m_imageCount);
    m_inFlightFences.resize(m_imageCount);
    m_imagesInFlight.resize(m_imageCount, VK_NULL_HANDLE);

    for (uint32_t i = 0; i < m_imageCount; ++i) {
        VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        if (vkCreateSemaphore(m_device->GetDevice(), &semInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create imageAvailable semaphore");
        if (vkCreateSemaphore(m_device->GetDevice(), &semInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create renderFinished semaphore");

        VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if (vkCreateFence(m_device->GetDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create inFlight fence");
    }
}

void Renderer::Run()
{
    m_running = m_window->PollEvents();

    // Acquire image
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device->GetDevice(), m_swapChain->GetSwapchain(), UINT64_MAX,
        m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // handle swapchain recreation (not implemented here)
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    // If a previous frame is using this image, wait on its fence
    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_device->GetDevice(), 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // mark the image as now being in use by this frame
    m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

    // Wait for this frame's fence to ensure CPU-GPU sync
    vkWaitForFences(m_device->GetDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_device->GetDevice(), 1, &m_inFlightFences[m_currentFrame]);

    // Reset and record command buffer for this image
    VkCommandBuffer cmd = m_commandBuffers->GetCommandBuffers()[imageIndex];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin command buffer");

    // --- Compute dispatch ---
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline->GetPipeline());
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline->GetLayout(), 0, 1, m_descriptorPool->GetComputeDescriptorSet(), 0, nullptr);

    // push constants for compute: time + numTriangles
    ComputePushConstants compPC{};
    compPC.time = static_cast<float>(clock() / static_cast<double>(CLOCKS_PER_SEC));
    compPC.numTriangles = static_cast<uint32_t>(m_triangles.size());
    vkCmdPushConstants(cmd, m_computePipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &compPC);

    vkCmdDispatch(cmd, (m_particleCount + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE, 1, 1);

    // memory barrier: make compute writes visible to vertex input (graphics)
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

    // Bind graphics pipeline & vertex buffer and draw
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline->GetPipeline());

    VkDeviceSize offsets[] = { 0 };
    VkBuffer vb = m_particleBuffer->Get();
    vkCmdBindVertexBuffers(cmd, 0, 1, &vb, offsets);

    // Push MVP for graphics (compute a basic MVP here; in your real app you'd compute camera/projection)
    GraphicsPushConstants gfxPC{};
    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
        (float)m_swapChain->GetExtent().width / (float)m_swapChain->GetExtent().height,
        0.1f, 100.0f);

    float angle = m_rotationSpeed * static_cast<float>(clock()) / CLOCKS_PER_SEC;
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

    // flip Y for vulkan NDC
    proj[1][1] *= -1.0f;
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    gfxPC.mvp = proj * view * model;
    vkCmdPushConstants(cmd, m_graphicsPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GraphicsPushConstants), &gfxPC);

    // draw
    vkCmdDraw(cmd, m_particleCount, 1, 0, 0);

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer");

    // Submit
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
        throw std::runtime_error("Failed to submit draw command buffer");

    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR scs[] = { m_swapChain->GetSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = scs;
    presentInfo.pImageIndices = &imageIndex;

    VkResult pres = vkQueuePresentKHR(m_device->GetPresentQueue(), &presentInfo);
    if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR) {
        // handle swapchain recreation (not implemented here)
    }
    else if (pres != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }

    m_currentFrame = (m_currentFrame + 1) % m_imageCount;
}

void Renderer::Shutdown()
{
    vkDeviceWaitIdle(m_device->GetDevice());

    for (uint32_t i = 0; i < m_imageCount; i++) {
        vkDestroySemaphore(m_device->GetDevice(), m_imageAvailable[i], nullptr);
        vkDestroySemaphore(m_device->GetDevice(), m_renderFinished[i], nullptr);
        vkDestroyFence(m_device->GetDevice(), m_inFlightFences[i], nullptr);
    }
}
