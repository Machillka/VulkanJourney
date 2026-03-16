#include "ChikaEngine/RHIResourceHandle.hpp"
#include "ChikaEngine/rhi/Vulkan/VulkanCommandList.hpp"
#include <ChikaEngine/rhi/Vulkan/VulkanRHIDevice.hpp>
#include <ChikaEngine/rhi/Vulkan/VulkanHelper.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <GLFW/glfw3.h>

namespace ChikaEngine::Render
{
    VulkanRHIDevice::VulkanRHIDevice() {}
    VulkanRHIDevice::~VulkanRHIDevice()
    {
        Shutdown();
    }

    void VulkanRHIDevice::Initialize(const RHI_InitParams& params)
    {
        std::cout << "[RHI_Vulkan] Initialize\n";

        CreateInstance(params.enableValidation);
        CreateSurface(params.nativeWindowHandle);
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapchain(params.width, params.height);
        CreateSwapchainImageViews();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
        CreateSwapchainRenderPass();
        CreateSwapchainFramebuffers();

        // 把 swapchain 的第 0 张图像包装成 mainRenderTarget
        // TODO: 打包
        uint32_t raw = AllocateHandleRaw();
        m_mainRenderTarget = RHI_TextureHandle(raw);

        TextureRecord rec{};
        if (!m_swapchainImages.empty())
        {
            rec.image = m_swapchainImages[0];
            rec.view = m_swapchainImageViews.empty() ? VK_NULL_HANDLE : m_swapchainImageViews[0];
        }
        rec.desc.width = m_swapchainExtent.width;
        rec.desc.height = m_swapchainExtent.height;
        rec.desc.format = RHI_Format::BGRA8_UNorm;
        rec.desc.usage = RHI_TextureUsage::Presentable;
        m_textures[raw] = rec;
    }
    void VulkanRHIDevice::Shutdown()
    {

        if (!m_device && !m_instance)
            return;

        vkDeviceWaitIdle(m_device);

        for (auto& pair : m_framebuffers)
            vkDestroyFramebuffer(m_device, pair.second.framebuffer, nullptr);
        m_framebuffers.clear();

        for (auto& pair : m_renderPasses)
            vkDestroyRenderPass(m_device, pair.second.renderPass, nullptr);
        m_renderPasses.clear();

        for (auto& pair : m_textures)
        {
            if (pair.second.view)
                vkDestroyImageView(m_device, pair.second.view, nullptr);
            if (pair.second.memory)
            {
                vkDestroyImage(m_device, pair.second.image, nullptr);
                vkFreeMemory(m_device, pair.second.memory, nullptr);
            }
        }
        m_textures.clear();

        for (auto& pair : m_buffers)
        {
            if (pair.second.buffer)
                vkDestroyBuffer(m_device, pair.second.buffer, nullptr);
            if (pair.second.memory)
                vkFreeMemory(m_device, pair.second.memory, nullptr);
        }
        m_buffers.clear();

        for (auto& pair : m_shaders)
            vkDestroyShaderModule(m_device, pair.second.module, nullptr);
        m_shaders.clear();

        for (auto& pair : m_pipelines)
        {
            if (pair.second.pipeline)
                vkDestroyPipeline(m_device, pair.second.pipeline, nullptr);
            if (pair.second.layout)
                vkDestroyPipelineLayout(m_device, pair.second.layout, nullptr);
        }
        m_pipelines.clear();

        // Clean up swapchain stuff
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (m_inFlightFences[i])
                vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
            if (m_imageAvailable[i])
                vkDestroySemaphore(m_device, m_imageAvailable[i], nullptr);
            if (m_renderFinished[i])
                vkDestroySemaphore(m_device, m_renderFinished[i], nullptr);
        }

        if (m_commandPool)
        {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (m_frameCommandPools[i])
            {
                vkDestroyCommandPool(m_device, m_frameCommandPools[i], nullptr);
                m_frameCommandPools[i] = VK_NULL_HANDLE;
            }
        }
        for (auto fb : m_swapchainFramebuffers)
            vkDestroyFramebuffer(m_device, fb, nullptr);
        m_swapchainFramebuffers.clear();

        if (m_swapchainRenderPass)
        {
            vkDestroyRenderPass(m_device, m_swapchainRenderPass, nullptr);
            m_swapchainRenderPass = VK_NULL_HANDLE;
        }

        for (auto view : m_swapchainImageViews)
            vkDestroyImageView(m_device, view, nullptr);
        m_swapchainImageViews.clear();

        if (m_swapchain)
        {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }

        if (m_surface)
        {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
            m_surface = VK_NULL_HANDLE;
        }

        if (m_device)
        {
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }

        if (m_instance)
        {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }

        std::cout << "[RHI_Vulkan] Shutdown\n";
    }

    void VulkanRHIDevice::BeginFrame()
    {
        VK_CHECK(vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX), "vkWaitForFences failed");
        VK_CHECK(vkResetCommandPool(m_device, m_frameCommandPools[m_currentFrame], 0), "vkResetCommandPool failed");

        VkResult res = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &m_currentImageIndex);
        if (res == VK_ERROR_OUT_OF_DATE_KHR)
            return;
        VK_CHECK(res, "vkAcquireNextImageKHR failed");

        VK_CHECK(vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]), "vkResetFences failed");
        VK_CHECK(vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0), "vkResetCommandBuffer failed");

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(m_commandBuffers[m_currentFrame], &begin), "vkBeginCommandBuffer failed");

        VkClearValue clear{};
        clear.color = {0.2f, 0.3f, 0.4f, 1.0f};

        VkRenderPassBeginInfo rp{};
        rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp.renderPass = m_swapchainRenderPass;
        rp.framebuffer = m_swapchainFramebuffers[m_currentImageIndex];
        rp.renderArea.offset = {0, 0};
        rp.renderArea.extent = m_swapchainExtent;
        rp.clearValueCount = 1;
        rp.pClearValues = &clear;

        vkCmdBeginRenderPass(m_commandBuffers[m_currentFrame], &rp, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(m_commandBuffers[m_currentFrame]);
    }

    void VulkanRHIDevice::EndFrame()
    {
        VK_CHECK(vkEndCommandBuffer(m_commandBuffers[m_currentFrame]), "vkEndCommandBuffer failed");

        VkSemaphore waitSemaphores[] = {m_imageAvailable[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSemaphore signalSemaphores[] = {m_renderFinished[m_currentFrame]};

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = waitSemaphores;
        submit.pWaitDstStageMask = waitStages;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &m_commandBuffers[m_currentFrame];
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = signalSemaphores;

        VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submit, m_inFlightFences[m_currentFrame]), "vkQueueSubmit failed");

        VkPresentInfoKHR present{};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = signalSemaphores;
        present.swapchainCount = 1;
        present.pSwapchains = &m_swapchain;
        present.pImageIndices = &m_currentImageIndex;

        VkResult res = vkQueuePresentKHR(m_graphicsQueue, &present);
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        {
        }
        else
        {
            VK_CHECK(res, "vkQueuePresentKHR failed");
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    RHI_BufferHandle VulkanRHIDevice::CreateBuffer(const RHI_BufferDesc& desc)
    {
        VkBufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = desc.size;
        info.usage = 0;
        if (desc.usage == RHI_BufferUsage::Vertex)
            info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (desc.usage == RHI_BufferUsage::Index)
            info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (desc.usage == RHI_BufferUsage::Uniform)
            info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        if (desc.usage == RHI_BufferUsage::Storage)
            info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer = VK_NULL_HANDLE;
        VK_CHECK(vkCreateBuffer(m_device, &info, nullptr, &buffer), "vkCreateBuffer failed");

        VkMemoryPropertyFlags props = 0;
        if (desc.cpuAccessible)
            props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        else
            props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VkDeviceMemory memory = VK_NULL_HANDLE;
        AllocateBufferMemory(buffer, props, memory);

        uint32_t raw = AllocateHandleRaw();
        RHI_BufferHandle h(raw);
        BufferRecord rec{};
        rec.buffer = buffer;
        rec.memory = memory;
        rec.desc = desc;
        m_buffers[raw] = rec;

        return h;
    }

    RHI_TextureHandle VulkanRHIDevice::CreateTexture(const RHI_TextureDesc& desc)
    {
        VkImageCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.extent.width = desc.width;
        info.extent.height = desc.height;
        info.extent.depth = 1;
        info.mipLevels = desc.mipLevels;
        info.arrayLayers = desc.arrayLayers;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = 0;
        if (desc.usage == RHI_TextureUsage::ColorAttachment)
            info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (desc.usage == RHI_TextureUsage::DepthStencilAttachment)
            info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (desc.usage == RHI_TextureUsage::Sampled)
            info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (desc.usage == RHI_TextureUsage::Storage)
            info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

        VkFormat format = ToVkFormat(desc.format);
        if (format == VK_FORMAT_UNDEFINED)
        {
            format = VK_FORMAT_R8G8B8A8_UNORM;
        }

        info.format = format;

        VkImage image = VK_NULL_HANDLE;
        VK_CHECK(vkCreateImage(m_device, &info, nullptr, &image), "vkCreateImage failed");

        VkDeviceMemory memory = VK_NULL_HANDLE;
        AllocateImageMemory(image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = desc.mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = desc.arrayLayers;

        if (format == VK_FORMAT_D32_SFLOAT)
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageView view = VK_NULL_HANDLE;
        VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &view), "vkCreateImageView failed");

        uint32_t raw = AllocateHandleRaw();
        RHI_TextureHandle h(raw);
        TextureRecord rec{};
        rec.image = image;
        rec.view = view;
        rec.memory = memory;
        rec.desc = desc;
        m_textures[raw] = rec;

        return h;
    }

    RHI_RenderPassHandle VulkanRHIDevice::CreateRenderPass(const RHI_RenderPassDesc& desc)
    {
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorRefs;
        VkAttachmentReference depthRef{};

        VkAttachmentDescription color{};
        color.format = ToVkFormat(desc.colorFormat);
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(color);

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorRefs.push_back(colorRef);

        if (desc.hasDepth)
        {
            VkAttachmentDescription depth{};
            depth.format = VK_FORMAT_D32_SFLOAT;
            depth.samples = VK_SAMPLE_COUNT_1_BIT;
            depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 通常深度不需要保存，丢弃可提升带宽性能
            depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments.push_back(depth);

            depthRef.attachment = 1; // 索引紧跟在 Color 后面
            depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
        subpass.pColorAttachments = colorRefs.data();
        if (desc.hasDepth)
            subpass.pDepthStencilAttachment = &depthRef;

        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = static_cast<uint32_t>(attachments.size());
        info.pAttachments = attachments.data();
        info.subpassCount = 1;
        info.pSubpasses = &subpass;

        VkRenderPass rp = VK_NULL_HANDLE;
        VK_CHECK(vkCreateRenderPass(m_device, &info, nullptr, &rp), "vkCreateRenderPass failed");

        uint32_t raw = AllocateHandleRaw();
        RHI_RenderPassHandle h(raw);
        RenderPassRecord rec{};
        rec.renderPass = rp;
        rec.desc = desc;
        m_renderPasses[raw] = rec;
        return h;
    }

    RHI_FramebufferHandle VulkanRHIDevice::CreateFramebuffer(const RHI_FramebufferDesc& desc)
    {
        auto itRP = m_renderPasses.find(desc.renderPass.raw_value);
        auto itTex = m_textures.find(desc.colorAttachment.raw_value);
        if (itRP == m_renderPasses.end() || itTex == m_textures.end())
            return RHI_FramebufferHandle::Invalid();

        std::vector<VkImageView> attachments;
        attachments.push_back(itTex->second.view);
        // VkImageView attachments[] = {itTex->second.view};
        if (desc.depthAttachment.IsValid())
        {
            auto itDepth = m_textures.find(desc.depthAttachment.raw_value);
            if (itDepth != m_textures.end())
                attachments.push_back(itDepth->second.view);
        }

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = itRP->second.renderPass;
        info.attachmentCount = static_cast<uint32_t>(attachments.size());
        info.pAttachments = attachments.data();
        info.width = desc.width;
        info.height = desc.height;
        info.layers = 1;

        VkFramebuffer fb = VK_NULL_HANDLE;
        VK_CHECK(vkCreateFramebuffer(m_device, &info, nullptr, &fb), "vkCreateFramebuffer failed");

        uint32_t raw = AllocateHandleRaw();
        RHI_FramebufferHandle h(raw);
        FramebufferRecord rec{};
        rec.framebuffer = fb;
        rec.desc = desc;
        m_framebuffers[raw] = rec;
        return h;
    }

    RHI_ShaderHandle VulkanRHIDevice::CreateShaderFromSPIRV(const void* data, size_t size)
    {
        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = size;
        info.pCode = reinterpret_cast<const uint32_t*>(data);

        VkShaderModule module = VK_NULL_HANDLE;
        VK_CHECK(vkCreateShaderModule(m_device, &info, nullptr, &module), "vkCreateShaderModule failed");

        uint32_t raw = AllocateHandleRaw();
        RHI_ShaderHandle h(raw);
        ShaderRecord rec{};
        rec.module = module;
        m_shaders[raw] = rec;
        return h;
    }

    void VulkanRHIDevice::UpdateBufferData(RHI_BufferHandle buffer, const void* data, size_t size) {}

    RHI_PipelineHandle VulkanRHIDevice::CreateGraphicsPipeline(const RHI_PipelineDesc& desc)
    {
        return RHI_PipelineHandle::Invalid();
    }

    std::unique_ptr<IRHICommandList> VulkanRHIDevice::CreateCommandList()
    {
        return std::make_unique<VulkanCommandList>(this);
    }

    VkPipelineLayout VulkanRHIDevice::GetVkPipelineLayout(RHI_PipelineHandle h) const
    {
        auto it = m_pipelines.find(h.raw_value);
        if (it == m_pipelines.end())
            return VK_NULL_HANDLE;
        return it->second.layout;
    }

    /*!
     * @brief  仅做提交
     *
     * @param  cmdList
     * @author Machillka (machillka2007@gmail.com)
     * @date 2026-03-14
     */
    void VulkanRHIDevice::SubmitCommandList(IRHICommandList* cmdList)
    {
        if (!cmdList)
            return;

        auto vkCmdList = dynamic_cast<VulkanCommandList*>(cmdList);
        if (!vkCmdList)
            throw std::runtime_error("SubmitCommandList: command list is not VulkanCommandList");

        VkCommandBuffer vkCmd = vkCmdList->GetVkCommandBuffer();
        if (vkCmd == VK_NULL_HANDLE)
            return;

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &vkCmd;

        VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submit, VK_NULL_HANDLE), "SubmitCommandList failed");
    }

    RHI_TextureHandle VulkanRHIDevice::GetMainRenderTarget() const
    {
        return m_mainRenderTarget;
    }

    void* VulkanRHIDevice::GetTextureRaw(RHI_TextureHandle tex) const
    {
        auto it = m_textures.find(tex.raw_value);
        if (it == m_textures.end())
            return nullptr;
        return reinterpret_cast<void*>(it->second.view);
    }

    // 查询接口实现
    VkBuffer VulkanRHIDevice::GetVkBuffer(RHI_BufferHandle h) const
    {
        auto it = m_buffers.find(h.raw_value);
        if (it == m_buffers.end())
            return VK_NULL_HANDLE;
        return it->second.buffer;
    }

    VkFramebuffer VulkanRHIDevice::GetVkFramebuffer(RHI_FramebufferHandle h) const
    {
        auto it = m_framebuffers.find(h.raw_value);
        if (it == m_framebuffers.end())
            return VK_NULL_HANDLE;
        return it->second.framebuffer;
    }

    VkRenderPass VulkanRHIDevice::GetVkRenderPass(RHI_RenderPassHandle h) const
    {
        auto it = m_renderPasses.find(h.raw_value);
        if (it == m_renderPasses.end())
            return VK_NULL_HANDLE;
        return it->second.renderPass;
    }

    VkPipeline VulkanRHIDevice::GetVkPipeline(RHI_PipelineHandle h) const
    {
        auto it = m_pipelines.find(h.raw_value);
        if (it == m_pipelines.end())
            return VK_NULL_HANDLE;
        return it->second.pipeline;
    }

    void VulkanRHIDevice::CreateInstance(bool enableValidation)
    {
        VkApplicationInfo app{};
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = "ChikaEngine";
        app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app.pEngineName = "ChikaEngine";
        app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app.apiVersion = VK_API_VERSION_1_2;

        uint32_t glfwExtCount = 0;
        // FIXME: 此处使用硬编码的 glfw
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

        std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);
        std::vector<const char*> layers;

        if (enableValidation)
        {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = &app;
        info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        info.enabledLayerCount = static_cast<uint32_t>(layers.size());
        info.ppEnabledLayerNames = layers.data();

        VK_CHECK(vkCreateInstance(&info, nullptr, &m_instance), "vkCreateInstance failed");
    }

    void VulkanRHIDevice::CreateSurface(void* windowHandle)
    {
        // FIXME: 硬编码使用 glfw
        auto glfwWindow = reinterpret_cast<GLFWwindow*>(windowHandle);
        if (glfwCreateWindowSurface(m_instance, glfwWindow, nullptr, &m_surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    void VulkanRHIDevice::PickPhysicalDevice()
    {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
        if (count == 0)
            throw std::runtime_error("No Vulkan physical device found");

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

        // TODO: 实现评估方法
        m_physicalDevice = devices[0];
    }

    void VulkanRHIDevice::CreateLogicalDevice()
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, families.data());

        m_graphicsQueueFamily = 0;
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                m_graphicsQueueFamily = i;
                break;
            }
        }

        float priority = 1.0f;
        VkDeviceQueueCreateInfo qinfo{};
        qinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qinfo.queueFamilyIndex = m_graphicsQueueFamily;
        qinfo.queueCount = 1;
        qinfo.pQueuePriorities = &priority;

        VkDeviceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &qinfo;

        const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        info.enabledExtensionCount = 1;
        info.ppEnabledExtensionNames = deviceExtensions;

        VK_CHECK(vkCreateDevice(m_physicalDevice, &info, nullptr, &m_device), "vkCreateDevice failed");
        vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);
    }

    void VulkanRHIDevice::CreateSwapchain(uint32_t width, uint32_t height)
    {
        VkSurfaceCapabilitiesKHR caps{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &caps);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

        VkSurfaceFormatKHR chosenFormat = formats[0];
        for (auto& f : formats)
        {
            if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                chosenFormat = f;
                break;
            }
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());

        VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR; // 一定可用

        VkExtent2D extent{};
        if (caps.currentExtent.width != UINT32_MAX)
        {
            extent = caps.currentExtent;
        }
        else
        {
            extent.width = std::max(caps.minImageExtent.width, std::min(caps.maxImageExtent.width, width));
            extent.height = std::max(caps.minImageExtent.height, std::min(caps.maxImageExtent.height, height));
        }

        uint32_t imageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
            imageCount = caps.maxImageCount;

        VkSwapchainCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = m_surface;
        info.minImageCount = imageCount;
        info.imageFormat = chosenFormat.format;
        info.imageColorSpace = chosenFormat.colorSpace;
        info.imageExtent = extent;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueFamilyIndices[] = {m_graphicsQueueFamily};
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 1;
        info.pQueueFamilyIndices = queueFamilyIndices;

        info.preTransform = caps.currentTransform;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = chosenPresentMode;
        info.clipped = VK_TRUE;
        info.oldSwapchain = VK_NULL_HANDLE;

        VK_CHECK(vkCreateSwapchainKHR(m_device, &info, nullptr, &m_swapchain), "vkCreateSwapchainKHR failed");

        m_swapchainFormat = chosenFormat.format;
        m_swapchainExtent = extent;

        uint32_t actualImageCount = 0;
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualImageCount, nullptr);
        m_swapchainImages.resize(actualImageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualImageCount, m_swapchainImages.data());
    }

    void VulkanRHIDevice::CreateSwapchainImageViews()
    {
        m_swapchainImageViews.resize(m_swapchainImages.size());

        for (size_t i = 0; i < m_swapchainImages.size(); ++i)
        {
            VkImageViewCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.image = m_swapchainImages[i];
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = m_swapchainFormat;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;

            VK_CHECK(vkCreateImageView(m_device, &info, nullptr, &m_swapchainImageViews[i]), "vkCreateImageView failed");
        }
    }

    void VulkanRHIDevice::CreateCommandPool()
    {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = m_graphicsQueueFamily;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CHECK(vkCreateCommandPool(m_device, &info, nullptr, &m_commandPool), "vkCreateCommandPool failed");

        info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VK_CHECK(vkCreateCommandPool(m_device, &info, nullptr, &m_frameCommandPools[i]), "vkCreateCommandPool (Frame) failed");
        }
    }

    void VulkanRHIDevice::CreateCommandBuffers()
    {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = m_commandPool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        VK_CHECK(vkAllocateCommandBuffers(m_device, &info, m_commandBuffers), "vkAllocateCommandBuffers failed");
    }

    void VulkanRHIDevice::CreateSyncObjects()
    {
        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        // 多帧
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VK_CHECK(vkCreateSemaphore(m_device, &semInfo, nullptr, &m_imageAvailable[i]), "vkCreateSemaphore failed");
            VK_CHECK(vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinished[i]), "vkCreateSemaphore failed");
            VK_CHECK(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]), "vkCreateFence failed");
        }
    }

    void VulkanRHIDevice::CreateSwapchainRenderPass()
    {
        VkAttachmentDescription color{};
        color.format = m_swapchainFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;

        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &color;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;

        VK_CHECK(vkCreateRenderPass(m_device, &info, nullptr, &m_swapchainRenderPass), "vkCreateRenderPass failed");
    }

    void VulkanRHIDevice::CreateSwapchainFramebuffers()
    {
        // TODO: 为每个 swapchain image view 创建 framebuffer
        m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

        for (size_t i = 0; i < m_swapchainImageViews.size(); ++i)
        {
            VkImageView attachments[] = {m_swapchainImageViews[i]};

            VkFramebufferCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.renderPass = m_swapchainRenderPass;
            info.attachmentCount = 1;
            info.pAttachments = attachments;
            info.width = m_swapchainExtent.width;
            info.height = m_swapchainExtent.height;
            info.layers = 1;

            VK_CHECK(vkCreateFramebuffer(m_device, &info, nullptr, &m_swapchainFramebuffers[i]), "vkCreateFramebuffer failed");
        }
    }

    void VulkanRHIDevice::AllocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags properties, VkDeviceMemory& outMemory)
    {
        VkMemoryRequirements req{};
        vkGetBufferMemoryRequirements(m_device, buffer, &req);

        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

        uint32_t typeIndex = UINT32_MAX;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        {
            if ((req.memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex == UINT32_MAX)
            throw std::runtime_error("No suitable memory type for buffer");

        VkMemoryAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize = req.size;
        alloc.memoryTypeIndex = typeIndex;

        VK_CHECK(vkAllocateMemory(m_device, &alloc, nullptr, &outMemory), "vkAllocateMemory failed");
        VK_CHECK(vkBindBufferMemory(m_device, buffer, outMemory, 0), "vkBindBufferMemory failed");
    }

    void VulkanRHIDevice::AllocateImageMemory(VkImage image, VkMemoryPropertyFlags properties, VkDeviceMemory& outMemory)
    {
        VkMemoryRequirements req{};
        vkGetImageMemoryRequirements(m_device, image, &req);

        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

        uint32_t typeIndex = UINT32_MAX;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        {
            if ((req.memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            {
                typeIndex = i;
                break;
            }
        }
        if (typeIndex == UINT32_MAX)
            throw std::runtime_error("No suitable memory type for image");

        VkMemoryAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize = req.size;
        alloc.memoryTypeIndex = typeIndex;

        VK_CHECK(vkAllocateMemory(m_device, &alloc, nullptr, &outMemory), "vkAllocateMemory failed");
        VK_CHECK(vkBindImageMemory(m_device, image, outMemory, 0), "vkBindImageMemory failed");
    }
} // namespace ChikaEngine::Render