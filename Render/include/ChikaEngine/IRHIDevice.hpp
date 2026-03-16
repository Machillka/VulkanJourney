#pragma once

#include "IRHICommandList.hpp"
#include "RHIDesc.hpp"
#include "RHIResourceHandle.hpp"
#include "RenderDevice.hpp"
#include <cstdint>
#include <memory>
namespace ChikaEngine::Render
{

    struct RHI_InitParams
    {
        void* nativeWindowHandle = nullptr;
        uint32_t width = 1280;
        uint32_t height = 720;
        bool enableValidation = true;
    };

    class IRHIDevice
    {
      public:
        virtual ~IRHIDevice() = default;

        // 初始化与销毁
        virtual void Initialize(const RHI_InitParams& params) = 0;
        virtual void Shutdown() = 0;

        // 每帧生命周期
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        // 资源创建（返回 RHI 句柄）
        virtual RHI_BufferHandle CreateBuffer(const RHI_BufferDesc& desc) = 0;
        virtual RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc) = 0;
        virtual RHI_RenderPassHandle CreateRenderPass(const RHI_RenderPassDesc& desc) = 0;
        virtual RHI_FramebufferHandle CreateFramebuffer(const RHI_FramebufferDesc& desc) = 0;

        // Shader / Pipeline / Descriptor（简化签名，后续扩展）
        virtual RHI_ShaderHandle CreateShaderFromSPIRV(const void* data, size_t size) = 0;
        virtual RHI_PipelineHandle CreateGraphicsPipeline(const RHI_PipelineDesc& desc) = 0;
        virtual void UpdateBufferData(RHI_BufferHandle buffer, const void* data, size_t size) = 0;

        // Create CommandList
        virtual std::unique_ptr<IRHICommandList> CreateCommandList() = 0;
        // 提交 Command 其中同步策略等全都交付给 RHI 实现
        virtual void SubmitCommandList(IRHICommandList* cmdList) = 0;

        virtual RHI_TextureHandle GetMainRenderTarget() const = 0;

        // 返回 输出纹理的 raw 使得 view windows 可以输出展示
        virtual void* GetTextureRaw(RHI_TextureHandle) const = 0;
        // virtual void* GetImGuiTextureID(RHI_TextureHandle tex) = 0;
    };
} // namespace ChikaEngine::Render