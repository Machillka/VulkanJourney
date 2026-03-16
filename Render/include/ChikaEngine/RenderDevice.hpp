/*!
 * @file RenderDevice.hpp
 * @author Machillka (machillka2007@gmail.com)
 * @brief  把来自 Renderer 的数据组装成 pass 然后提交给 IRHI
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "ChikaEngine/math/mat4.h"
#include "ChikaEngine/math/vector3.h"
#include "ChikaEngine/math/vector4.h"
#include "IResourceView.hpp"
#include "IRHICommandList.hpp"
#include "RHIDesc.hpp"
#include "RHIResourceHandle.hpp"
#include "RenderResourceHandle.hpp"
#include <memory>
#include <vector>
namespace ChikaEngine::Render
{
    // 前向声明硬件层
    class IRHIDevice;
    class ResourceBinder;
    struct MeshDrawCommand
    {
        MeshHandle mesh;
        MaterialHandle material;
        Math::Mat4 worldTransform;
    };

    struct SkyboxCommand
    {
        TextureHandle cubemap;
        bool valid = false;
    };

    struct GizmoLine
    {
        Math::Vector3 a;
        Math::Vector3 b;
        Math::Vector4 color;
    };

    // 提供输出的纹理描述即可
    struct RenderDeviceDesc
    {
        RHI_TextureDesc mainTargetDesc;
    };

    class RenderDevice
    {
      public:
        // rhi: 指向已初始化的 RHI 实例（由 Renderer 在 Init 时创建并传入）
        // resourceView: 指向资源系统提供的只读视图
        RenderDevice(IRHIDevice* rhi, const IResourceView* resourceView, const RenderDeviceDesc& desc);
        ~RenderDevice();

        // 每帧生命周期
        void BeginFrame();
        void EndFrame();

        // 提交接口（Renderer 调用）
        void SubmitMesh(const MeshDrawCommand& cmd);
        void SubmitSkybox(const SkyboxCommand& cmd);
        void SubmitGizmoLine(const GizmoLine& line);

        // 执行渲染（内部调用 RHI）
        void RenderFrame();

        void SetCamera(const CameraData& cameraData);

        // 获取主渲染目标（供 Editor 使用）
        RHI_TextureHandle GetMainRenderTarget() const;

      private:
        IRHIDevice* m_rhi;
        const IResourceView* m_resourceView;

        CameraData m_camera;

        std::vector<MeshDrawCommand> m_meshCommands;
        SkyboxCommand m_skybox;
        std::vector<GizmoLine> m_gizmoLines;

        std::unique_ptr<ResourceBinder> m_binder;

        RHI_TextureHandle m_mainRenderTarget;
        RHI_TextureHandle m_depthRenderTarget;
        RHI_RenderPassHandle m_mainRenderPass;
        RHI_FramebufferHandle m_mainFramebuffer;

        // 向 RHI 发送指令
        // 直接 pull 数据然后缓存下来？
        void ExecuteSkyboxPass(std::unique_ptr<IRHICommandList>& cmdList);
        void ExecuteScenePass(std::unique_ptr<IRHICommandList>& cmdList);
        void ExecuteGizmoPass(std::unique_ptr<IRHICommandList>& cmdList);

        RHI_PipelineHandle m_scenePipeline = RHI_PipelineHandle::Invalid();
    };

} // namespace ChikaEngine::Render