#include "RenderPass.h"

// STD
#include <stdexcept>

// VULKAN
#include <vulkan/vulkan.h>

RenderPass::RenderPass(Device* device, Swapchain* swapchain)
	: m_Device(device->GetDevice()), m_Extent(swapchain->GetExtent()), m_swapchain(swapchain)
{
	CreateRenderPass(swapchain->GetFormat());
	CreateFramebuffers(swapchain->GetImageViews());
}

RenderPass::~RenderPass()
{
    for (auto fb : m_Framebuffers) 
    {
        vkDestroyFramebuffer(m_Device, fb, nullptr);
    }

    if (m_renderPass != VK_NULL_HANDLE) 
    {
        vkDestroyRenderPass(m_Device, m_renderPass, nullptr);
    }
}

void RenderPass::Begin(VkCommandBuffer cmd, uint32_t imageIndex)
{
    VkRenderPassBeginInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderInfo.renderPass = m_renderPass;
	renderInfo.framebuffer = GetFramebuffers()[imageIndex];
    renderInfo.renderArea.offset = { 0,0 };
    renderInfo.renderArea.extent = m_swapchain->GetExtent();
    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderInfo.clearValueCount = 1;
    renderInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass::CreateRenderPass(VkFormat swapchainFormat)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void RenderPass::CreateFramebuffers(const std::vector<VkImageView>& imageViews)
{
    m_Framebuffers.resize(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); ++i) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &imageViews[i];
        framebufferInfo.width = m_Extent.width;
        framebufferInfo.height = m_Extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}
