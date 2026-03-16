#pragma once

#include "ChikaEngine/RHIDesc.hpp"
#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
namespace ChikaEngine::Render
{
    // 单独对 Vulkan Check 的检查封装
    // TODO: 接入 Chika Log System
    static void VK_CHECK(VkResult res, const char* msg)
    {
        if (res != VK_SUCCESS)
        {
            std::cerr << "[Vulkan] Error: " << msg << " VkResult=" << res << "\n";
            throw std::runtime_error(msg);
        }
    }

    /*!
     * @brief  把 RHI 枚举转化成 Vk
     *
     * @param  fmt
     * @return VkFormat
     * @author Machillka (machillka2007@gmail.com)
     * @date 2026-03-13
     */
    static VkFormat ToVkFormat(RHI_Format fmt)
    {
        switch (fmt)
        {
        case RHI_Format::RGBA8_UNorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case RHI_Format::BGRA8_UNorm:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case RHI_Format::RGBA16_Float:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case RHI_Format::R32_Float:
            return VK_FORMAT_R32_SFLOAT;
        case RHI_Format::D24S8:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case RHI_Format::D32_SFloat:
            return VK_FORMAT_D32_SFLOAT;
        default:
            return VK_FORMAT_UNDEFINED;
        }
    }
} // namespace ChikaEngine::Render