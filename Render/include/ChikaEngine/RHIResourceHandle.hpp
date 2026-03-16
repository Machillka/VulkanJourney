#pragma once

#include <ChikaEngine/base/HandleTemplate.h>

namespace ChikaEngine::Render
{
    struct RHI_BufferTag;
    struct RHI_TextureTag;
    struct RHI_ShaderTag;
    struct RHI_PipelineTag;
    struct RHI_DescriptorSetTag;
    struct RHI_RenderPassTag;
    struct RHI_FramebufferTag;

    // NOTE: 和 Vulkan 一样, 例如 vertex mesh等资源, 都是作为 Buffer 传入
    using RHI_BufferHandle = Core::THandle<RHI_BufferTag>;
    using RHI_TextureHandle = Core::THandle<RHI_TextureTag>;
    using RHI_ShaderHandle = Core::THandle<RHI_ShaderTag>;
    using RHI_PipelineHandle = Core::THandle<RHI_PipelineTag>;
    using RHI_DescriptorSetHandle = Core::THandle<RHI_DescriptorSetTag>;
    using RHI_RenderPassHandle = Core::THandle<RHI_RenderPassTag>;
    using RHI_FramebufferHandle = Core::THandle<RHI_FramebufferTag>;

}; // namespace ChikaEngine::Render