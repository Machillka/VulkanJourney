#include "ChikaEngine/ResourceBinder.hpp"
#include "ChikaEngine/IRHIDevice.hpp"

#include <cstdint>

namespace ChikaEngine::Render
{

    ResourceBinder::ResourceBinder(IRHIDevice* rhi, const IResourceView* resourceView) : m_rhi(rhi), m_resourceView(resourceView)
    {
        if (!m_rhi)
            throw std::runtime_error("ResourceBinder: IRHI is null");
        if (!m_resourceView)
            throw std::runtime_error("ResourceBinder: IResourceView is null");
    }

    ResourceBinder::~ResourceBinder()
    {
        Clear();
    }

    // TODO: 使用队列实现
    RHI_BufferHandle ResourceBinder::BindVertexBuffer(MeshHandle mesh)
    {
        std::lock_guard lock(m_mutex);
        uint32_t key = mesh.raw_value;

        auto it = m_vertexMap.find(key);
        if (it != m_vertexMap.end())
        {
            m_refCounts[key]++;
            return it->second;
        }

        const MeshDataView* view = m_resourceView->GetMesh(mesh);
        if (!view)
            return RHI_BufferHandle::Invalid();
        RHI_BufferDesc desc{};
        desc.size = view->vertexSizeBytes;
        desc.usage = RHI_BufferUsage::Vertex;
        desc.cpuAccessible = false;

        RHI_BufferHandle rhiBuf = m_rhi->CreateBuffer(desc);
        if (rhiBuf == RHI_BufferHandle::Invalid())
            return rhiBuf;

        m_vertexMap[key] = rhiBuf;
        m_refCounts[key] = 1;
        return rhiBuf;
    }

    RHI_BufferHandle ResourceBinder::BindIndexBuffer(MeshHandle mesh)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        uint32_t key = mesh.raw_value;
        auto it = m_indexMap.find(key);
        if (it != m_indexMap.end())
        {
            m_refCounts[key]++;
            return it->second;
        }

        const MeshDataView* view = m_resourceView->GetMesh(mesh);
        if (!view || view->indexSizeBytes == 0)
            return RHI_BufferHandle::Invalid();

        RHI_BufferDesc desc{};
        desc.size = view->indexSizeBytes;
        desc.usage = RHI_BufferUsage::Index;
        desc.cpuAccessible = false;

        RHI_BufferHandle rhiBuf = m_rhi->CreateBuffer(desc);
        if (rhiBuf == RHI_BufferHandle::Invalid())
            return rhiBuf;

        m_indexMap[key] = rhiBuf;
        m_refCounts[key] = 1;
        return rhiBuf;
    }

    RHI_TextureHandle ResourceBinder::BindTexture(TextureHandle tex)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        uint32_t key = tex.raw_value;
        auto it = m_textureMap.find(key);
        if (it != m_textureMap.end())
        {
            m_refCounts[key]++;
            return it->second;
        }

        const TextureDataView* view = m_resourceView->GetTexture(tex);
        if (!view)
            return RHI_TextureHandle::Invalid();

        RHI_TextureDesc desc{};
        desc.width = view->width;
        desc.height = view->height;
        // TODO: 可以自定义
        desc.format = RHI_Format::RGBA8_UNorm;
        desc.mipLevels = view->mipLevels;
        desc.arrayLayers = 1;
        desc.usage = RHI_TextureUsage::Sampled;

        RHI_TextureHandle rhiTex = m_rhi->CreateTexture(desc);
        if (rhiTex == RHI_TextureHandle::Invalid())
            return rhiTex;

        m_textureMap[key] = rhiTex;
        m_refCounts[key] = 1;
        return rhiTex;
    }

    RHI_ShaderHandle ResourceBinder::BindShader(ShaderHandle shader)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        uint32_t key = shader.raw_value;
        auto it = m_shaderMap.find(key);
        if (it != m_shaderMap.end())
        {
            m_refCounts[key]++;
            return it->second;
        }

        const ShaderDataView* view = m_resourceView->GetShader(shader);
        if (!view || !view->spirvData || view->spirvSize == 0)
            return RHI_ShaderHandle::Invalid();

        RHI_ShaderHandle rhiShader = m_rhi->CreateShaderFromSPIRV(view->spirvData, view->spirvSize);
        if (rhiShader == RHI_ShaderHandle::Invalid())
            return rhiShader;

        m_shaderMap[key] = rhiShader;
        m_refCounts[key] = 1;
        return rhiShader;
    }

    void ResourceBinder::UnbindMesh(MeshHandle mesh)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        uint32_t key = mesh.raw_value;
        auto it = m_refCounts.find(key);
        if (it == m_refCounts.end())
            return;
        // 仅剩此一个
        if (--(it->second) == 0)
        {
            m_refCounts.erase(it);
            m_vertexMap.erase(key);
            m_indexMap.erase(key);
        }
    }

    void ResourceBinder::UnbindTexture(TextureHandle tex)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        uint32_t key = tex.raw_value;
        auto it = m_refCounts.find(key);
        if (it == m_refCounts.end())
            return;
        if (--(it->second) == 0)
        {
            m_refCounts.erase(it);
            m_textureMap.erase(key);
        }
    }

    void ResourceBinder::Clear()
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_vertexMap.clear();
        m_indexMap.clear();
        m_textureMap.clear();
        m_shaderMap.clear();
        m_refCounts.clear();
    }

} // namespace ChikaEngine::Render
