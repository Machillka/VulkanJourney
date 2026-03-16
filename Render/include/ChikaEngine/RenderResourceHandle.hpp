#pragma once

#include <ChikaEngine/base/HandleTemplate.h>
#include "ChikaEngine/math/mat4.h"
#include "ChikaEngine/math/vector3.h"

namespace ChikaEngine::Render
{
    struct MeshTag;
    struct MaterialTag;
    struct TextureTag;
    struct ShaderTag;

    using MeshHandle = Core::THandle<MeshTag>;
    using MaterialHandle = Core::THandle<MaterialTag>;
    using TextureHandle = Core::THandle<TextureTag>;
    using ShaderHandle = Core::THandle<ShaderTag>;

    struct CameraData
    {
        Math::Mat4 view;
        Math::Mat4 proj;
        Math::Mat4 viewProj;
        Math::Vector3 position;
        float padding0;
    };
}; // namespace ChikaEngine::Render