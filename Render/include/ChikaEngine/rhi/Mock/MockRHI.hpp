#pragma once

#include "ChikaEngine/IRHICommandList.hpp"
#include "ChikaEngine/IRHIDevice.hpp"
#include <iostream>
#include <unordered_map>

namespace ChikaEngine::Render
{
    class MockCommandList : public IRHICommandList
    {
      public:
        void Begin() override
        {
            std::cout << "[MockRHI] CommandList Begin\n";
        }
        void End() override
        {
            std::cout << "[MockRHI] CommandList End\n";
        }
        void SetViewport(float, float) override {}
        void PushConstants(RHI_PipelineHandle pipeline, uint32_t stageFlags, uint32_t offset, uint32_t size, const void* data) override {}
        void BeginRenderPass(RHI_RenderPassHandle, RHI_FramebufferHandle, const float clearColor[4]) override
        {
            std::cout << "[MockRHI] BeginRenderPass clear=(" << clearColor[0] << "," << clearColor[1] << "," << clearColor[2] << "," << clearColor[3] << ")\n";
        }

        void EndRenderPass() override
        {
            std::cout << "[MockRHI] EndRenderPass\n";
        }

        void BindPipeline(RHI_PipelineHandle) override
        {
            std::cout << "[MockRHI] BindPipeline\n";
        }
        void BindVertexBuffer(RHI_BufferHandle, uint64_t) override
        {
            std::cout << "[MockRHI] BindVertexBuffer\n";
        }
        void BindIndexBuffer(RHI_BufferHandle, uint64_t) override
        {
            std::cout << "[MockRHI] BindIndexBuffer\n";
        }
        void BindDescriptorSet(RHI_DescriptorSetHandle, uint32_t) override
        {
            std::cout << "[MockRHI] BindDescriptorSet\n";
        }

        void Draw(uint32_t, uint32_t) override
        {
            std::cout << "[MockRHI] Draw\n";
        }
        void DrawIndexed(uint32_t, uint32_t, int32_t) override
        {
            std::cout << "[MockRHI] DrawIndexed\n";
        }
    };

    class MockRHI : public IRHIDevice
    {
      public:
        MockRHI() : m_nextRaw(1) {}
        ~MockRHI() override = default;
        void UpdateBufferData(RHI_BufferHandle buffer, const void* data, size_t size) override {}
        void Initialize(const RHI_InitParams& params) override
        {
            std::cout << "[MockRHI] Initialize: window=" << params.nativeWindowHandle << " size=" << params.width << "x" << params.height << "\n";
        }

        void Shutdown() override
        {
            std::cout << "[MockRHI] Shutdown\n";
        }

        void BeginFrame() override
        {
            std::cout << "[MockRHI] BeginFrame\n";
        }

        void EndFrame() override
        {
            std::cout << "[MockRHI] EndFrame\n";
        }

        RHI_BufferHandle CreateBuffer(const RHI_BufferDesc& desc) override
        {
            uint32_t raw = m_nextRaw++;
            RHI_BufferHandle h(raw);
            m_buffers[raw] = desc;
            std::cout << "[MockRHI] CreateBuffer size=" << desc.size << "\n";
            return h;
        }

        RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc) override
        {
            uint32_t raw = m_nextRaw++;
            RHI_TextureHandle h(raw);
            m_textures[raw] = desc;
            std::cout << "[MockRHI] CreateTexture " << desc.width << "x" << desc.height << "\n";
            return h;
        }

        RHI_RenderPassHandle CreateRenderPass(const RHI_RenderPassDesc&) override
        {
            uint32_t raw = m_nextRaw++;
            RHI_RenderPassHandle h(raw);
            std::cout << "[MockRHI] CreateRenderPass\n";
            return h;
        }

        RHI_FramebufferHandle CreateFramebuffer(const RHI_FramebufferDesc&) override
        {
            uint32_t raw = m_nextRaw++;
            RHI_FramebufferHandle h(raw);
            std::cout << "[MockRHI] CreateFramebuffer\n";
            return h;
        }

        RHI_ShaderHandle CreateShaderFromSPIRV(const void*, size_t) override
        {
            uint32_t raw = m_nextRaw++;
            RHI_ShaderHandle h(raw);
            std::cout << "[MockRHI] CreateShaderFromSPIRV\n";
            return h;
        }

        RHI_PipelineHandle CreateGraphicsPipeline(const RHI_PipelineDesc& desc) override
        {
            uint32_t raw = m_nextRaw++;
            RHI_PipelineHandle h(raw);
            std::cout << "[MockRHI] CreateGraphicsPipeline\n";
            return h;
        }

        std::unique_ptr<IRHICommandList> CreateCommandList() override
        {
            return std::make_unique<MockCommandList>();
        }

        RHI_TextureHandle GetMainRenderTarget() const override
        {
            if (m_textures.empty())
                return RHI_TextureHandle::Invalid();
            return RHI_TextureHandle(m_textures.begin()->first);
        }
        void* GetTextureRaw(RHI_TextureHandle tex) const override
        {
            // NOTE: 占位 实际上可能只需要得到 Main Texture 的 Raw Data 即可
            return reinterpret_cast<void*>(static_cast<uintptr_t>(tex.raw_value));
        }

        void SubmitCommandList(IRHICommandList* cmdList) override
        {
            std::cout << "[MockRHI] SubmitCommandList\n";
        }

      private:
        std::atomic<uint32_t> m_nextRaw;
        std::unordered_map<uint32_t, RHI_BufferDesc> m_buffers;
        std::unordered_map<uint32_t, RHI_TextureDesc> m_textures;
        std::unordered_map<uint32_t, RHI_RenderPassDesc> m_renderPasses;
        std::unordered_map<uint32_t, RHI_FramebufferDesc> m_framebuffers;
    };
} // namespace ChikaEngine::Render
