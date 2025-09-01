// EntryPoint.cpp  -- fixed / refactored version of your EntryPoint()
// Assumes Window, Instance, Device, Swapchain, RenderPass, CommandBuffers have the members used below.

#include "EntryPoint.h"
#include "Window.h"
#include "Instance.h"
#include "Device.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include "CommandBuffers.h"

#include <vulkan/vulkan.h>
#include "tinyobjloader/tiny_obj_loader.h"

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <array>
#include <iostream>

namespace Core {

    static std::vector<char> ReadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Failed to open file: " + filename);

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module!");

        return shaderModule;
    }

    struct Triangle {
        glm::vec4 v0;
        glm::vec4 v1;
        glm::vec4 v2;
    };

    struct ComputePushConstants {
        float time;
        uint32_t numTriangles;
    };

    struct GraphicsPushConstants {
        glm::mat4 mvp;
    };

    // Helper: find memory type index (keeps your previous pattern)
    static uint32_t FindMemoryTypeIndex(VkPhysicalDevice physical, uint32_t typeFilter, VkMemoryPropertyFlags props) {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physical, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
                return i;
        }
        throw std::runtime_error("Failed to find suitable memory type index");
    }

    int EntryPoint()
    {
        // --- Window / Vulkan setup ---
        Window window("GPU Point Cloud", 1280, 720);
        Instance instance(&window);
        VkSurfaceKHR surface = instance.CreateVulkanSurface(&window);
        Device device(instance.Get(), surface);
        Swapchain swapchain(&device, &window);
        RenderPass renderPass(&device, &swapchain);

        // --- Load OBJ triangles ---
        std::vector<Triangle> triangles;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "objects/Suzanne.obj"))
            throw std::runtime_error(warn + err);

        for (const auto& shape : shapes) {
            for (size_t f = 0; f < shape.mesh.indices.size(); f += 3) {
                tinyobj::index_t i0 = shape.mesh.indices[f + 0];
                tinyobj::index_t i1 = shape.mesh.indices[f + 1];
                tinyobj::index_t i2 = shape.mesh.indices[f + 2];

                triangles.push_back({
                    glm::vec4(attrib.vertices[3 * i0.vertex_index + 0], attrib.vertices[3 * i0.vertex_index + 1], attrib.vertices[3 * i0.vertex_index + 2], 0),
                    glm::vec4(attrib.vertices[3 * i1.vertex_index + 0], attrib.vertices[3 * i1.vertex_index + 1], attrib.vertices[3 * i1.vertex_index + 2], 0),
                    glm::vec4(attrib.vertices[3 * i2.vertex_index + 0], attrib.vertices[3 * i2.vertex_index + 1], attrib.vertices[3 * i2.vertex_index + 2], 0)
                    });
            }
        }

        if (triangles.empty())
            throw std::runtime_error("No triangles loaded from OBJ.");

        // --- Triangle storage buffer (host visible) ---
        VkBuffer triangleBuffer = VK_NULL_HANDLE;
        VkDeviceMemory triangleMemory = VK_NULL_HANDLE;

        VkDevice deviceHandle = device.GetDevice();
        VkPhysicalDevice physical = device.GetPhysicalDevice();

        VkBufferCreateInfo triBufferInfo{};
        triBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        triBufferInfo.size = sizeof(Triangle) * triangles.size();
        triBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        triBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(deviceHandle, &triBufferInfo, nullptr, &triangleBuffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create triangle buffer!");

        VkMemoryRequirements triMemReq;
        vkGetBufferMemoryRequirements(deviceHandle, triangleBuffer, &triMemReq);

        VkMemoryAllocateInfo triAlloc{};
        triAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        triAlloc.allocationSize = triMemReq.size;
        triAlloc.memoryTypeIndex = FindMemoryTypeIndex(physical, triMemReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(deviceHandle, &triAlloc, nullptr, &triangleMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate triangle buffer memory!");
        vkBindBufferMemory(deviceHandle, triangleBuffer, triangleMemory, 0);

        // upload triangle data
        void* triDst = nullptr;
        vkMapMemory(deviceHandle, triangleMemory, 0, triBufferInfo.size, 0, &triDst);
        memcpy(triDst, triangles.data(), triBufferInfo.size);
        vkUnmapMemory(deviceHandle, triangleMemory);

        // --- Point buffer (device local) ---
        const uint32_t numPoints = 10000;
        VkBuffer pointBuffer = VK_NULL_HANDLE;
        VkDeviceMemory pointMemory = VK_NULL_HANDLE;

        VkBufferCreateInfo pointInfo{};
        pointInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        pointInfo.size = sizeof(glm::vec4) * numPoints; // use vec4 for alignment
        pointInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        pointInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(deviceHandle, &pointInfo, nullptr, &pointBuffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create point buffer!");

        VkMemoryRequirements pointMemReq;
        vkGetBufferMemoryRequirements(deviceHandle, pointBuffer, &pointMemReq);

        VkMemoryAllocateInfo pointAlloc{};
        pointAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        pointAlloc.allocationSize = pointMemReq.size;
        // Try DEVICE_LOCAL first; if that fails on allocation, you would fallback to HOST_VISIBLE in real code.
        pointAlloc.memoryTypeIndex = FindMemoryTypeIndex(physical, pointMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(deviceHandle, &pointAlloc, nullptr, &pointMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate point buffer memory!");
        vkBindBufferMemory(deviceHandle, pointBuffer, pointMemory, 0);

        // --- Command pool for graphics (we record both compute+graphics into the same primary cmd) ---
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = device.GetGraphicsFamilyIndex();
        VkCommandPool commandPool = VK_NULL_HANDLE;
        if (vkCreateCommandPool(deviceHandle, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool!");

        // CommandBuffers utility: creates per-swapchain-image command buffers for recording render pass commands.
        CommandBuffers cmdBuffers(deviceHandle, commandPool,
            renderPass.GetRenderPass(),
            renderPass.GetFramebuffers(),
            swapchain.GetExtent());

        // --- Descriptor set layout for compute ---
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

        VkDescriptorSetLayout descSetLayout;
        if (vkCreateDescriptorSetLayout(deviceHandle, &descLayoutInfo, nullptr, &descSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout!");

        // --- Compute pipeline layout (with push constants for time + numTriangles) ---
        VkPushConstantRange compPush{};
        compPush.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        compPush.offset = 0;
        compPush.size = sizeof(ComputePushConstants);

        VkPipelineLayoutCreateInfo compLayoutInfo{};
        compLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        compLayoutInfo.setLayoutCount = 1;
        compLayoutInfo.pSetLayouts = &descSetLayout;
        compLayoutInfo.pushConstantRangeCount = 1;
        compLayoutInfo.pPushConstantRanges = &compPush;

        VkPipelineLayout compPipelineLayout;
        if (vkCreatePipelineLayout(deviceHandle, &compLayoutInfo, nullptr, &compPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute pipeline layout!");

        // --- Descriptor pool + allocate descriptor set ---
        VkDescriptorPoolSize poolSizes[1];
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[0].descriptorCount = 2;

        VkDescriptorPoolCreateInfo poolCreate{};
        poolCreate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreate.poolSizeCount = 1;
        poolCreate.pPoolSizes = poolSizes;
        poolCreate.maxSets = 1;

        VkDescriptorPool descPool;
        if (vkCreateDescriptorPool(deviceHandle, &poolCreate, nullptr, &descPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool!");

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descSetLayout;

        VkDescriptorSet computeDescSet;
        if (vkAllocateDescriptorSets(deviceHandle, &allocInfo, &computeDescSet) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor set!");

        VkDescriptorBufferInfo triBufInfo{};
        triBufInfo.buffer = triangleBuffer;
        triBufInfo.offset = 0;
        triBufInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo pointBufInfo{};
        pointBufInfo.buffer = pointBuffer;
        pointBufInfo.offset = 0;
        pointBufInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet writeTri{};
        writeTri.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeTri.dstSet = computeDescSet;
        writeTri.dstBinding = 0;
        writeTri.dstArrayElement = 0;
        writeTri.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeTri.descriptorCount = 1;
        writeTri.pBufferInfo = &triBufInfo;

        VkWriteDescriptorSet writePoint{};
        writePoint.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writePoint.dstSet = computeDescSet;
        writePoint.dstBinding = 1;
        writePoint.dstArrayElement = 0;
        writePoint.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writePoint.descriptorCount = 1;
        writePoint.pBufferInfo = &pointBufInfo;

        std::array<VkWriteDescriptorSet, 2> writeSets = { writeTri, writePoint };
        vkUpdateDescriptorSets(deviceHandle, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

        // --- Create compute pipeline (once) ---
        auto compCode = ReadFile("shaders/pointcloud.comp.spv");
        VkShaderModule compShader = CreateShaderModule(deviceHandle, compCode);

        VkPipelineShaderStageCreateInfo compStage{};
        compStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        compStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        compStage.module = compShader;
        compStage.pName = "main";

        VkComputePipelineCreateInfo computePipeInfo{};
        computePipeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipeInfo.stage = compStage;
        computePipeInfo.layout = compPipelineLayout;

        VkPipeline compPipeline;
        if (vkCreateComputePipelines(deviceHandle, VK_NULL_HANDLE, 1, &computePipeInfo, nullptr, &compPipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create compute pipeline!");
        // we keep compShader until cleanup (or destroy it now)
        vkDestroyShaderModule(deviceHandle, compShader, nullptr);

        // --- Create graphics pipeline (once) ---
        // read shader SPIR-V
        auto vertCode = ReadFile("shaders/pointcloud.vert.spv");
        auto fragCode = ReadFile("shaders/pointcloud.frag.spv");
        VkShaderModule vertShader = CreateShaderModule(deviceHandle, vertCode);
        VkShaderModule fragShader = CreateShaderModule(deviceHandle, fragCode);

        // graphics pipeline layout: push constant for MVP
        VkPushConstantRange gfxPush{};
        gfxPush.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        gfxPush.offset = 0;
        gfxPush.size = sizeof(GraphicsPushConstants);

        VkPipelineLayoutCreateInfo gfxPLInfo{};
        gfxPLInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        gfxPLInfo.setLayoutCount = 0;
        gfxPLInfo.pSetLayouts = nullptr;
        gfxPLInfo.pushConstantRangeCount = 1;
        gfxPLInfo.pPushConstantRanges = &gfxPush;

        VkPipelineLayout graphicsPipelineLayout = VK_NULL_HANDLE;
        if (vkCreatePipelineLayout(deviceHandle, &gfxPLInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline layout!");

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertShader;
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragShader;
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(glm::vec4); // we wrote vec4 into SSBO
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attrDesc{};
        attrDesc.binding = 0;
        attrDesc.location = 0;
        attrDesc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrDesc.offset = 0;

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &bindingDesc;
        vertexInput.vertexAttributeDescriptionCount = 1;
        vertexInput.pVertexAttributeDescriptions = &attrDesc;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchain.GetExtent().width;
        viewport.height = (float)swapchain.GetExtent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0,0 };
        scissor.extent = swapchain.GetExtent();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisample{};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.sampleShadingEnable = VK_FALSE;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = graphicsPipelineLayout;
        pipelineInfo.renderPass = renderPass.GetRenderPass();
        pipelineInfo.subpass = 0;

        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(deviceHandle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline!");
        // shaders can be destroyed after pipeline creation
        vkDestroyShaderModule(deviceHandle, vertShader, nullptr);
        vkDestroyShaderModule(deviceHandle, fragShader, nullptr);

        // --- Synchronization objects (per-frame) ---
        uint32_t imageCount = static_cast<uint32_t>(swapchain.GetImageViews().size());
        std::vector<VkSemaphore> imageAvailable(imageCount);
        std::vector<VkSemaphore> renderFinished(imageCount);
        std::vector<VkFence> inFlightFences(imageCount);
        std::vector<VkFence> imagesInFlight(imageCount, VK_NULL_HANDLE);

        for (uint32_t i = 0; i < imageCount; ++i) {
            VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            if (vkCreateSemaphore(deviceHandle, &semInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create imageAvailable semaphore");
            if (vkCreateSemaphore(deviceHandle, &semInfo, nullptr, &renderFinished[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create renderFinished semaphore");

            VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            if (vkCreateFence(deviceHandle, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create inFlight fence");
        }

        // --- Per-frame command buffer allocation: We'll reuse the CommandBuffers class to get per-image command buffers.
        // (Assuming CommandBuffers already has recorded arrays sized to swapchain images and returns VkCommandBuffer)
        // We'll re-record per-frame command buffer (compute+render) into the buffer for the acquired image.

        // --- Main loop ---
        bool running = true;
        uint32_t currentFrame = 0;

        // compute group size must match your compute shader local_size_x
        const uint32_t workGroupSize = 256;
        const uint32_t numGroups = (numPoints + workGroupSize - 1) / workGroupSize;

        while (running) {
            // Poll window events using your Window class
            running = window.PollEvents();

            // Acquire image
            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(deviceHandle, swapchain.GetSwapchain(), UINT64_MAX,
                imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                // handle swapchain recreation (not implemented here)
                break;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw std::runtime_error("Failed to acquire swapchain image");
            }

            // If a previous frame is using this image, wait on its fence
            if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
                vkWaitForFences(deviceHandle, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
            }
            // mark the image as now being in use by this frame
            imagesInFlight[imageIndex] = inFlightFences[currentFrame];

            // Wait for this frame's fence to ensure CPU-GPU sync
            vkWaitForFences(deviceHandle, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
            vkResetFences(deviceHandle, 1, &inFlightFences[currentFrame]);

            // Reset and record command buffer for this image
            VkCommandBuffer cmd = cmdBuffers.GetCommandBuffers()[imageIndex];
            vkResetCommandBuffer(cmd, 0);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;
            if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS)
                throw std::runtime_error("Failed to begin command buffer");

            // --- Compute dispatch ---
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compPipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compPipelineLayout, 0, 1, &computeDescSet, 0, nullptr);

            // push constants for compute: time + numTriangles
            ComputePushConstants compPC{};
            compPC.time = static_cast<float>(clock() / static_cast<double>(CLOCKS_PER_SEC));
            compPC.numTriangles = static_cast<uint32_t>(triangles.size());
            vkCmdPushConstants(cmd, compPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &compPC);

            vkCmdDispatch(cmd, (numPoints + workGroupSize - 1) / workGroupSize, 1, 1);

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

            // --- Graphics pass ---
            VkRenderPassBeginInfo renderInfo{};
            renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderInfo.renderPass = renderPass.GetRenderPass();
            renderInfo.framebuffer = renderPass.GetFramebuffers()[imageIndex];
            renderInfo.renderArea.offset = { 0,0 };
            renderInfo.renderArea.extent = swapchain.GetExtent();
            VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
            renderInfo.clearValueCount = 1;
            renderInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(cmd, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Bind graphics pipeline & vertex buffer and draw
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd, 0, 1, &pointBuffer, offsets);

            // Push MVP for graphics (compute a basic MVP here; in your real app you'd compute camera/projection)
            GraphicsPushConstants gfxPC{};
            glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                (float)swapchain.GetExtent().width / (float)swapchain.GetExtent().height,
                0.1f, 100.0f);
            // flip Y for vulkan NDC
            proj[1][1] *= -1.0f;
            glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            gfxPC.mvp = proj * view;
            vkCmdPushConstants(cmd, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GraphicsPushConstants), &gfxPC);

            // draw
            vkCmdDraw(cmd, numPoints, 1, 0, 0);

            vkCmdEndRenderPass(cmd);

            if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
                throw std::runtime_error("Failed to record command buffer");

            // Submit
            VkSemaphore waitSemaphores[] = { imageAvailable[currentFrame] };
            VkSemaphore signalSemaphores[] = { renderFinished[currentFrame] };
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

            if (vkQueueSubmit(device.GetGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
                throw std::runtime_error("Failed to submit draw command buffer");

            // Present
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;
            VkSwapchainKHR scs[] = { swapchain.GetSwapchain() };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = scs;
            presentInfo.pImageIndices = &imageIndex;

            VkResult pres = vkQueuePresentKHR(device.GetPresentQueue(), &presentInfo);
            if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR) {
                // handle swapchain recreation (not implemented here)
            }
            else if (pres != VK_SUCCESS) {
                throw std::runtime_error("Failed to present swapchain image");
            }

            currentFrame = (currentFrame + 1) % imageCount;
        }

        // wait and cleanup
        vkDeviceWaitIdle(deviceHandle);

        for (uint32_t i = 0; i < imageCount; i++) {
            vkDestroySemaphore(deviceHandle, imageAvailable[i], nullptr);
            vkDestroySemaphore(deviceHandle, renderFinished[i], nullptr);
            vkDestroyFence(deviceHandle, inFlightFences[i], nullptr);
        }

        vkDestroyPipeline(deviceHandle, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(deviceHandle, graphicsPipelineLayout, nullptr);

        vkDestroyPipeline(deviceHandle, compPipeline, nullptr);
        vkDestroyPipelineLayout(deviceHandle, compPipelineLayout, nullptr);

        vkDestroyDescriptorPool(deviceHandle, descPool, nullptr);
        vkDestroyDescriptorSetLayout(deviceHandle, descSetLayout, nullptr);

        vkDestroyBuffer(deviceHandle, triangleBuffer, nullptr);
        vkFreeMemory(deviceHandle, triangleMemory, nullptr);

        vkDestroyBuffer(deviceHandle, pointBuffer, nullptr);
        vkFreeMemory(deviceHandle, pointMemory, nullptr);

        vkDestroyCommandPool(deviceHandle, commandPool, nullptr);

        return 0;
    }

} // namespace Core
