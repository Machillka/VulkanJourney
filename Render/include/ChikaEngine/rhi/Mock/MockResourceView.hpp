#pragma once

#include "ChikaEngine/IResourceView.hpp"

#include <vector>
#include <cstdint>
#include <cstring>

namespace ChikaEngine::Render::Test
{
    class MockResourceView : public IResourceView
    {
      public:
        MockResourceView()
        {
            float verts[9] = {0.0f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f};
            m_vertexData.resize(sizeof(verts));
            std::memcpy(m_vertexData.data(), verts, sizeof(verts));

            m_mesh.vertexData = m_vertexData.data();
            m_mesh.vertexSizeBytes = m_vertexData.size();
            m_mesh.indexData = nullptr;
            m_mesh.indexSizeBytes = 0;
            m_mesh.vertexCount = 3;
            m_mesh.indexCount = 0;
            m_mesh.vertexStride = 3 * sizeof(float);
        }

        const MeshDataView* GetMesh(MeshHandle) const override
        {
            return &m_mesh;
        }

        const TextureDataView* GetTexture(TextureHandle) const override
        {
            return nullptr;
        }

        const ShaderDataView* GetShader(ShaderHandle) const override
        {
            return nullptr;
        }

      private:
        MeshDataView m_mesh;
        std::vector<std::uint8_t> m_vertexData;
    };
} // namespace ChikaEngine::Render::Test