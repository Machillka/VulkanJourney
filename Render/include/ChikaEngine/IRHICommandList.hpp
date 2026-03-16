/*!
 * @file RHICommandList.hpp
 * @author Machillka (machillka2007@gmail.com)
 * @brief  抽象命令列表接口， Command buffer 功能，GPU 指令缓冲区
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "RHIResourceHandle.hpp"
#include <cstdint>
namespace ChikaEngine::Render
{

    class IRHICommandList
    {
      public:
        virtual ~IRHICommandList() = default;

        // Begin/End 命令录制
        virtual void Begin() = 0;
        virtual void End() = 0;

        // RenderPass 控制
        virtual void BeginRenderPass(RHI_RenderPassHandle renderPass, RHI_FramebufferHandle framebuffer, const float clearColor[4]) = 0;
        virtual void EndRenderPass() = 0;

        virtual void PushConstants(RHI_PipelineHandle pipeline, uint32_t stageFlags, uint32_t offset, uint32_t size, const void* data) = 0;
        virtual void SetViewport(float width, float height) = 0;

        virtual void BindPipeline(RHI_PipelineHandle pipeline) = 0;
        virtual void BindVertexBuffer(RHI_BufferHandle buffer, uint64_t offset = 0) = 0;
        virtual void BindIndexBuffer(RHI_BufferHandle buffer, uint64_t offset = 0) = 0;
        virtual void BindDescriptorSet(RHI_DescriptorSetHandle set, uint32_t setIndex = 0) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t firstVertex = 0) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t firstIndex = 0, int32_t vertexOffset = 0) = 0;
    };
} // namespace ChikaEngine::Render