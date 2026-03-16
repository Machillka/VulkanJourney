#pragma once

#include "ChikaEngine/math/vector4.h"
#include "IResourceView.hpp"
#include "RenderDevice.hpp"
#include "RenderResourceHandle.hpp"
#include "rhi/RHIBackendFactory.hpp"
#include <ChikaEngine/base/HandleTemplate.h>
#include <cstdint>
#include <memory>

namespace ChikaEngine::Render
{

    /*!
     * @brief  偷学 Vulkan 的字段命名 用于初始化创建 Render
     *
     * @author Machillka (machillka2007@gmail.com)
     * @date 2026-03-13
     */
    struct RendererCreateInfo
    {
        void* windowHandle = nullptr;
        uint32_t width = 1280;
        uint32_t height = 720;

        RHIBackendTypes backend = RHIBackendTypes::Auto;
        const IResourceView* resourceView = nullptr;
    };

    class Renderer
    {
      public:
        static Renderer& Instance()
        {
            static Renderer instance;
            return instance;
        }

        void Init(const RendererCreateInfo& createInfo);
        void Shutdown();

        // Life cycle
        void BeginFrame();
        void EndFrame();

        void SetCamera(const CameraData& cameraData);

        // 收集数据
        void SubmitMesh(MeshHandle mesh, MaterialHandle material, const Math::Mat4& worldTransform);
        void SubmitSkybox(TextureHandle cubemap);
        void SubmitGizmoLine(const Math::Vector3& a, const Math::Vector3& b, const Math::Vector4& color);

        RHI_TextureHandle GetViewportTexture() const;
        void RegisterResourceView(const IResourceView* view);
        void* GetTextureRaw() const;

      private:
        Renderer() = default;
        ~Renderer() = default;
        std::unique_ptr<IRHIDevice> m_rhi;
        std::unique_ptr<RenderDevice> m_renderDevice = nullptr;
        const IResourceView* m_resourceView = nullptr;
        RendererCreateInfo m_params;

        // 本地缓存相机数据
        ChikaEngine::Math::Mat4 m_view;
        ChikaEngine::Math::Mat4 m_proj;
    };

} // namespace ChikaEngine::Render