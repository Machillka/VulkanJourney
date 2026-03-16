#pragma once

#include "RenderResourceHandle.hpp"
#include <cstdint>
#include <vector>

namespace ChikaEngine::Render
{

    struct VertexAttributeDesc
    {
        uint32_t location; // shader location
        uint32_t offset;   // byte offset in vertex struct
        uint32_t format;   // custom enum for attribute format
    };

    struct MeshDataView
    {
        // 通过 void* 指针匹配所有后端类型
        const void* vertexData = nullptr;
        size_t vertexSizeBytes = 0;
        const void* indexData = nullptr;
        size_t indexSizeBytes = 0;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t vertexStride = 0;
        std::vector<VertexAttributeDesc> attributes;
    };

    struct TextureDataView
    {
        const void* pixelData = nullptr;
        size_t pixelSizeBytes = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevels = 1;
    };

    struct ShaderDataView
    {
        const void* spirvData = nullptr;
        size_t spirvSize = 0;
    };

    class IResourceView
    {
      public:
        virtual ~IResourceView() = default;

        virtual const MeshDataView* GetMesh(MeshHandle h) const = 0;
        virtual const TextureDataView* GetTexture(TextureHandle h) const = 0;
        virtual const ShaderDataView* GetShader(ShaderHandle h) const = 0;
    };
} // namespace ChikaEngine::Render