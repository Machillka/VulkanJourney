#pragma once

#include "IResourceView.hpp"
#include "RHIResourceHandle.hpp"
#include "RenderDevice.hpp"
#include <mutex>
#include <unordered_map>
namespace ChikaEngine::Render
{

    class ResourceBinder
    {
      public:
        ResourceBinder(IRHIDevice* rhi, const IResourceView* resourceView);
        ~ResourceBinder();

        // Bind and return RHI handle (cached)
        RHI_BufferHandle BindVertexBuffer(MeshHandle mesh);
        RHI_BufferHandle BindIndexBuffer(MeshHandle mesh);
        RHI_TextureHandle BindTexture(TextureHandle tex);
        RHI_ShaderHandle BindShader(ShaderHandle shader);

        // Unbind (decrease refcount)
        void UnbindMesh(MeshHandle mesh);
        void UnbindTexture(TextureHandle tex);

        // Clear all caches (e.g., on RHI device lost)
        void Clear();

      private:
        IRHIDevice* m_rhi;
        const IResourceView* m_resourceView;

        // 渲染层级的资源缓存
        std::unordered_map<uint32_t, RHI_BufferHandle> m_vertexMap;
        std::unordered_map<uint32_t, RHI_BufferHandle> m_indexMap;
        std::unordered_map<uint32_t, RHI_TextureHandle> m_textureMap;
        std::unordered_map<uint32_t, RHI_ShaderHandle> m_shaderMap;

        std::unordered_map<uint32_t, uint32_t> m_refCounts;

        std::mutex m_mutex;
    };
} // namespace ChikaEngine::Render