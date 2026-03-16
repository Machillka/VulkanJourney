#include <ChikaEngine/rhi/Vulkan/VulkanCommandList.hpp>
#include <ChikaEngine/rhi/Vulkan/VulkanHelper.hpp>

namespace ChikaEngine::Render
{

    VulkanCommandList::VulkanCommandList(IRHIDevice* device)
    {
        m_device = static_cast<VulkanRHIDevice*>(device);
        if (!m_device)
            throw std::runtime_error("VulkanCommandList: device is not VulkanRHIDevice");
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = m_device->GetFrameCommandPool();
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = 1;

        VK_CHECK(vkAllocateCommandBuffers(m_device->GetVkDevice(), &info, &m_cmd), "vkAllocateCommandBuffers failed");
    }

    VulkanCommandList::~VulkanCommandList()
    {
        if (m_cmd)
        {
            // vkFreeCommandBuffers(m_device->GetVkDevice(), m_device->GetCommandPool(), 1, &m_cmd);
            m_cmd = VK_NULL_HANDLE;
        }
    }

    void VulkanCommandList::Begin()
    {
        VkCommandBufferBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(m_cmd, &info), "vkBeginCommandBuffer failed");
        m_recording = true;
    }

    void VulkanCommandList::End()
    {
        VK_CHECK(vkEndCommandBuffer(m_cmd), "vkEndCommandBuffer failed");
        m_recording = false;
    }

    void VulkanCommandList::BeginRenderPass(RHI_RenderPassHandle renderPass, RHI_FramebufferHandle framebuffer, const float clearColor[4])
    {
        VkRenderPass rp = m_device->GetVkRenderPass(renderPass);
        VkFramebuffer fb = m_device->GetVkFramebuffer(framebuffer);
        if (rp == VK_NULL_HANDLE || fb == VK_NULL_HANDLE)
            return;

        std::vector<VkClearValue> clearValues(2);
        clearValues[0].color = {{clearColor[0], clearColor[1], clearColor[2], clearColor[3]}};
        clearValues[1].depthStencil = {1.0f, 0}; // 深度清空为 1.0 (最远)

        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = rp;
        info.framebuffer = fb;
        info.renderArea.offset = {0, 0};
        info.renderArea.extent = {1280, 720}; // TODO: 从 FramebufferRecord.desc 取
        info.clearValueCount = static_cast<uint32_t>(clearValues.size());
        info.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_cmd, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanCommandList::EndRenderPass()
    {
        vkCmdEndRenderPass(m_cmd);
    }

    void VulkanCommandList::BindPipeline(RHI_PipelineHandle pipeline)
    {
        VkPipeline p = m_device->GetVkPipeline(pipeline);
        if (p == VK_NULL_HANDLE)
            return;
        vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p);
    }

    void VulkanCommandList::BindVertexBuffer(RHI_BufferHandle buffer, uint64_t offset)
    {
        VkBuffer buf = m_device->GetVkBuffer(buffer);
        if (buf == VK_NULL_HANDLE)
            return;
        VkDeviceSize off = offset;
        vkCmdBindVertexBuffers(m_cmd, 0, 1, &buf, &off);
    }

    void VulkanCommandList::BindIndexBuffer(RHI_BufferHandle buffer, uint64_t offset)
    {
        VkBuffer buf = m_device->GetVkBuffer(buffer);
        if (buf == VK_NULL_HANDLE)
            return;
        vkCmdBindIndexBuffer(m_cmd, buf, offset, VK_INDEX_TYPE_UINT32);
    }

    void VulkanCommandList::BindDescriptorSet(RHI_DescriptorSetHandle, uint32_t) {}

    void VulkanCommandList::Draw(uint32_t vertexCount, uint32_t firstVertex)
    {
        vkCmdDraw(m_cmd, vertexCount, 1, firstVertex, 0);
    }

    void VulkanCommandList::DrawIndexed(uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset)
    {
        vkCmdDrawIndexed(m_cmd, indexCount, 1, firstIndex, vertexOffset, 0);
    }

    void VulkanCommandList::PushConstants(RHI_PipelineHandle pipeline, uint32_t stageFlags, uint32_t offset, uint32_t size, const void* data)
    {
        VkPipelineLayout layout = m_device->GetVkPipelineLayout(pipeline); // 需在 Device 添加查询接口
        if (layout == VK_NULL_HANDLE)
            return;

        VkShaderStageFlags flags = 0;
        if (stageFlags == 1)
            flags = VK_SHADER_STAGE_VERTEX_BIT; // 简单的掩码映射

        vkCmdPushConstants(m_cmd, layout, flags, offset, size, data);
    }
    void VulkanCommandList::SetViewport(float width, float height)
    {
        VkViewport viewport{};
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent.width = (uint32_t)width;
        scissor.extent.height = (uint32_t)height;
        vkCmdSetScissor(m_cmd, 0, 1, &scissor);
    }
} // namespace ChikaEngine::Render