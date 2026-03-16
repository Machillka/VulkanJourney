#pragma once

#include "ChikaEngine/IRHICommandList.hpp"
#include "ChikaEngine/IRHIDevice.hpp"
#include "VulkanRHIDevice.hpp"
#include <vulkan/vulkan.h>

namespace ChikaEngine::Render
{

    class VulkanCommandList : public IRHICommandList
    {
      public:
        VulkanCommandList(IRHIDevice* device);
        ~VulkanCommandList() override;
        void Begin() override;
        void End() override;

        void BeginRenderPass(RHI_RenderPassHandle renderPass, RHI_FramebufferHandle framebuffer, const float clearColor[4]) override;
        void EndRenderPass() override;

        void BindPipeline(RHI_PipelineHandle pipeline) override;
        void BindVertexBuffer(RHI_BufferHandle buffer, uint64_t offset = 0) override;
        void BindIndexBuffer(RHI_BufferHandle buffer, uint64_t offset = 0) override;
        void BindDescriptorSet(RHI_DescriptorSetHandle set, uint32_t setIndex = 0) override;

        void Draw(uint32_t vertexCount, uint32_t firstVertex = 0) override;
        void DrawIndexed(uint32_t indexCount, uint32_t firstIndex = 0, int32_t vertexOffset = 0) override;

        void PushConstants(RHI_PipelineHandle pipeline, uint32_t stageFlags, uint32_t offset, uint32_t size, const void* data) override;
        void SetViewport(float width, float height) override;
        VkCommandBuffer GetVkCommandBuffer() const
        {
            return m_cmd;
        }

      private:
        VulkanRHIDevice* m_device = nullptr;
        VkCommandBuffer m_cmd = VK_NULL_HANDLE;
        bool m_recording = false;
    };
} // namespace ChikaEngine::Render