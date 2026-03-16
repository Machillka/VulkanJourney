/*!
 * @file RHIDesc.hpp
 * @author Machillka (machillka2007@gmail.com)
 * @brief  RHI 资源描述 可以后续拓展
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "RHIResourceHandle.hpp"
#include <cstdint>
#include <vector>
namespace ChikaEngine::Render
{
    enum class RHI_Format : uint32_t
    {
        Unknown = 0,
        RGBA8_UNorm,
        BGRA8_UNorm,
        RGBA16_Float,
        R32_Float,
        D32_SFloat,
        D24S8,
    };

    enum class RHI_BufferUsage : uint32_t
    {
        Vertex,
        Index,
        Uniform,
        Storage,
        Staging,
        TransferDst
    };

    enum class RHI_TextureUsage : uint32_t
    {
        ColorAttachment,
        DepthStencilAttachment,
        Sampled,
        Storage,
        Presentable
    };

    struct RHI_BufferDesc
    {
        uint64_t size = 0;
        RHI_BufferUsage usage = RHI_BufferUsage::Vertex;
        bool cpuAccessible = false; // 是否需要 host visible
    };

    struct RHI_TextureDesc
    {
        uint32_t width = 0;
        uint32_t height = 0;
        RHI_Format format = RHI_Format::RGBA8_UNorm;
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;
        RHI_TextureUsage usage = RHI_TextureUsage::Sampled;
    };

    struct RHI_RenderPassDesc
    {
        RHI_Format colorFormat = RHI_Format::RGBA8_UNorm;
        bool hasDepth = false;
        RHI_Format depthFormat = RHI_Format::D24S8;
    };

    struct RHI_FramebufferDesc
    {
        RHI_RenderPassHandle renderPass;
        RHI_TextureHandle colorAttachment;
        RHI_TextureHandle depthAttachment;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    // 枚举 Shader 的类型
    enum class RHI_ShaderStage : uint32_t
    {
        Vertex,
        Fragment,
        Compute
    };

    struct RHI_DescriptorSetLayoutDesc
    {
        struct Binding
        {
            uint32_t binding;
            RHI_ShaderStage stage;
            bool isUBO; // UBO | SSBO
        };
        std::vector<Binding> bindings;
    };

    struct RHI_VertexAttribute
    {
        uint32_t location;
        RHI_Format format;
        uint32_t offset;
    };

    // 顶点描述符
    struct RHI_VertexLayout
    {
        uint32_t stride;
        std::vector<RHI_VertexAttribute> attributes;
    };

    /*!
     * @brief  单个管线描述
     * 需要 vert 和 frag shader
     * 同时需要指定 顶点布局
     *
     * @author Machillka (machillka2007@gmail.com)
     * @date 2026-03-16
     */
    struct RHI_PipelineDesc
    {
        RHI_ShaderHandle vertexShader;
        RHI_ShaderHandle fragmentShader;
        RHI_VertexLayout vertexLayout;
        RHI_RenderPassHandle renderPass;
        bool depthTest = true;
        bool depthWrite = true;
    };

} // namespace ChikaEngine::Render