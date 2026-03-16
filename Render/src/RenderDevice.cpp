#include "ChikaEngine/RenderDevice.hpp"
#include "ChikaEngine/IRHIDevice.hpp"
#include "ChikaEngine/ResourceBinder.hpp"
#include <iostream>
#include <stdexcept>

namespace ChikaEngine::Render
{
    RenderDevice::RenderDevice(IRHIDevice* rhi, const IResourceView* resourceView, const RenderDeviceDesc& desc) : m_rhi(rhi), m_resourceView(resourceView)
    {
        // NOTE: 搬回 ChikaEngine 改成自定义 Debug 系统
        if (!m_rhi)
            throw std::runtime_error("RenderDevice: IRHI is null");
        if (!m_resourceView)
            throw std::runtime_error("RenderDevice: IResourceView is null");

        m_binder = std::make_unique<ResourceBinder>(m_rhi, m_resourceView);

        uint32_t width = desc.mainTargetDesc.width > 0 ? desc.mainTargetDesc.width : 1280;
        uint32_t height = desc.mainTargetDesc.height > 0 ? desc.mainTargetDesc.height : 720;

        RHI_TextureDesc colorDesc{};
        colorDesc.width = width;
        colorDesc.height = height;
        colorDesc.format = desc.mainTargetDesc.format;
        colorDesc.mipLevels = 1;
        colorDesc.arrayLayers = 1;
        colorDesc.usage = RHI_TextureUsage::ColorAttachment;

        m_mainRenderTarget = m_rhi->CreateTexture(colorDesc);
        if (!m_mainRenderTarget.IsValid())
            throw std::runtime_error("RenderDevice: failed to create main render target");

        RHI_TextureDesc depthDesc{};
        depthDesc.width = width;
        depthDesc.height = height;
        depthDesc.format = RHI_Format::D32_SFloat;
        depthDesc.mipLevels = 1;
        depthDesc.arrayLayers = 1;
        depthDesc.usage = RHI_TextureUsage::DepthStencilAttachment;

        m_depthRenderTarget = m_rhi->CreateTexture(depthDesc);
        if (!m_depthRenderTarget.IsValid())
            throw std::runtime_error("RenderDevice: failed to create depth render target");

        RHI_RenderPassDesc rpDesc{};
        rpDesc.colorFormat = colorDesc.format;
        rpDesc.hasDepth = true; // LINUS: 开启深度！
        m_mainRenderPass = m_rhi->CreateRenderPass(rpDesc);

        RHI_FramebufferDesc fbDesc{};
        fbDesc.renderPass = m_mainRenderPass;
        fbDesc.colorAttachment = m_mainRenderTarget;  // 颜色
        fbDesc.depthAttachment = m_depthRenderTarget; // 深度
        fbDesc.width = width;
        fbDesc.height = height;
        m_mainFramebuffer = m_rhi->CreateFramebuffer(fbDesc);
    }

    RenderDevice::~RenderDevice()
    {
        m_binder.reset();
    }

    // 开始的时候清空命令
    void RenderDevice::BeginFrame()
    {
        m_meshCommands.clear();
        m_gizmoLines.clear();
        m_skybox = SkyboxCommand{};
    }

    void RenderDevice::EndFrame()
    {
        RenderFrame();
    }
    void RenderDevice::SubmitMesh(const MeshDrawCommand& cmd)
    {
        m_meshCommands.push_back(cmd);
    }

    void RenderDevice::SubmitSkybox(const SkyboxCommand& cmd)
    {
        m_skybox = cmd;
    }

    void RenderDevice::SubmitGizmoLine(const GizmoLine& line)
    {
        m_gizmoLines.push_back(line);
    }
    RHI_TextureHandle RenderDevice::GetMainRenderTarget() const
    {
        return m_mainRenderTarget;
    }

    void RenderDevice::RenderFrame()
    {
        auto cmdList = m_rhi->CreateCommandList();
        if (!cmdList)
            return;

        ExecuteScenePass(cmdList);

        cmdList->End();

        m_rhi->SubmitCommandList(cmdList.get());
    }

    void RenderDevice::ExecuteSkyboxPass(std::unique_ptr<IRHICommandList>& cmdList)
    {
        // TODO: 绑定 skybox pipeline + cubemap
    }

    void RenderDevice::ExecuteScenePass(std::unique_ptr<IRHICommandList>& cmdList)
    {

        if (!m_mainRenderPass.IsValid() || !m_mainFramebuffer.IsValid())
            return;

        cmdList->Begin();

        const float clearColor[4] = {0.1f, 0.2f, 0.2f, 1.0f};
        cmdList->BeginRenderPass(m_mainRenderPass, m_mainFramebuffer, clearColor);

        cmdList->SetViewport(1280.0f, 720.0f);

        cmdList->BindPipeline(m_scenePipeline);

        for (const auto& draw : m_meshCommands)
        {
            RHI_BufferHandle vb = m_binder->BindVertexBuffer(draw.mesh);
            RHI_BufferHandle ib = m_binder->BindIndexBuffer(draw.mesh);

            if (!vb.IsValid())
                continue;

            cmdList->BindVertexBuffer(vb, 0);
            if (ib.IsValid())
                cmdList->BindIndexBuffer(ib, 0);

            Math::Mat4 mvp = m_camera.proj * m_camera.view * draw.worldTransform;

            cmdList->PushConstants(m_scenePipeline, 1, 0, sizeof(Math::Mat4), &mvp);

            const MeshDataView* meshView = m_resourceView->GetMesh(draw.mesh);
            if (meshView->indexCount > 0 && ib.IsValid())
            {
                cmdList->DrawIndexed(meshView->indexCount, 0, 0);
            }
            else
            {
                cmdList->Draw(meshView->vertexCount, 0);
            }
        }

        cmdList->EndRenderPass();
    }

    void RenderDevice::ExecuteGizmoPass(std::unique_ptr<IRHICommandList>& cmdList)
    {
        // TODO: 用 line pipeline 绘制 gizmo
        if (!m_gizmoLines.empty())
            std::cout << "[RenderDevice] ExecuteGizmoPass: " << m_gizmoLines.size() << " lines (placeholder)\n";
    }

    void RenderDevice::SetCamera(const CameraData& cameraData)
    {
        m_camera = cameraData;
    }
} // namespace ChikaEngine::Render
