#include "ChikaEngine/Renderer.hpp"
#include <iostream>
#include <memory>

namespace ChikaEngine::Render
{

    void Renderer::Init(const RendererCreateInfo& params)
    {
        if (!params.windowHandle)
            throw std::runtime_error("Renderer::Init: windowHandle is null");
        if (!params.resourceView)
            throw std::runtime_error("Renderer::Init: resourceView is null");
        m_params = params;
        m_resourceView = params.resourceView;

        m_rhi = RHIBackendFactory::CreateRHIDevice(params.backend);
        if (!m_rhi)
            throw std::runtime_error("Renderer::Init: RHIFactory::Create returned null");

        RHI_InitParams rhiParams{};
        rhiParams.nativeWindowHandle = params.windowHandle;
        rhiParams.width = params.width;
        rhiParams.height = params.height;
        rhiParams.enableValidation = true;
        m_rhi->Initialize(rhiParams);

        RenderDeviceDesc devDesc{};
        devDesc.mainTargetDesc.width = params.width;
        devDesc.mainTargetDesc.height = params.height;
        devDesc.mainTargetDesc.format = RHI_Format::RGBA8_UNorm;
        devDesc.mainTargetDesc.mipLevels = 1;
        devDesc.mainTargetDesc.arrayLayers = 1;
        devDesc.mainTargetDesc.usage = RHI_TextureUsage::ColorAttachment;

        m_renderDevice = std::make_unique<RenderDevice>(m_rhi.get(), m_resourceView, devDesc);

        m_view = ChikaEngine::Math::Mat4::Identity();
        m_proj = ChikaEngine::Math::Mat4::Identity();

        std::cout << "[Renderer] Initialized with backend=" << static_cast<int>(params.backend) << "\n";
    }

    void Renderer::Shutdown()
    {
        m_renderDevice.reset();
        m_rhi = nullptr;
        m_resourceView = nullptr;
        std::cout << "[Renderer] Shutdown.\n";
    }

    void Renderer::BeginFrame()
    {
        if (m_rhi)
            m_rhi->BeginFrame();
        if (m_renderDevice)
            m_renderDevice->BeginFrame();
    }

    void Renderer::EndFrame()
    {
        if (m_renderDevice)
            m_renderDevice->EndFrame();
        if (m_rhi)
            m_rhi->EndFrame();
    }

    void Renderer::SetCamera(const CameraData& cameraData)
    {
        m_view = cameraData.view;
        m_proj = cameraData.proj;

        if (m_renderDevice)
            m_renderDevice->SetCamera(cameraData);
    }

    void Renderer::SubmitMesh(MeshHandle mesh, MaterialHandle material, const ChikaEngine::Math::Mat4& world)
    {
        if (!m_renderDevice)
            return;
        MeshDrawCommand cmd{};
        cmd.mesh = mesh;
        cmd.material = material;
        cmd.worldTransform = world;
        m_renderDevice->SubmitMesh(cmd);
    }

    void Renderer::SubmitSkybox(TextureHandle cubemap)
    {
        if (!m_renderDevice)
            return;
        SkyboxCommand cmd{};
        cmd.cubemap = cubemap;
        cmd.valid = true;
        m_renderDevice->SubmitSkybox(cmd);
    }

    void Renderer::SubmitGizmoLine(const ChikaEngine::Math::Vector3& a, const ChikaEngine::Math::Vector3& b, const ChikaEngine::Math::Vector4& color)
    {
        if (!m_renderDevice)
            return;
        GizmoLine line{};
        line.a = a;
        line.b = b;
        line.color = color;
        m_renderDevice->SubmitGizmoLine(line);
    }

    RHI_TextureHandle Renderer::GetViewportTexture() const
    {
        if (!m_renderDevice)
            return RHI_TextureHandle::Invalid();
        return m_renderDevice->GetMainRenderTarget();
    }

    void* Renderer::GetTextureRaw() const
    {
        if (!m_rhi || !m_renderDevice)
            return nullptr;
        auto tex = m_renderDevice->GetMainRenderTarget();
        return m_rhi->GetTextureRaw(tex);
    }
} // namespace ChikaEngine::Render