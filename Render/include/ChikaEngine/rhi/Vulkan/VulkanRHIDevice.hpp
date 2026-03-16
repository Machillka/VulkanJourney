#pragma once

#include "ChikaEngine/IRHIDevice.hpp"
#include <unordered_map>
#include <vulkan/vulkan.h>
namespace ChikaEngine::Render
{

    class VulkanRHIDevice : public IRHIDevice
    {
      public:
        VulkanRHIDevice();
        ~VulkanRHIDevice() override;

        void Initialize(const RHI_InitParams& params) override;
        void Shutdown() override;

        void BeginFrame() override;
        void EndFrame() override;

        RHI_BufferHandle CreateBuffer(const RHI_BufferDesc& desc) override;
        RHI_TextureHandle CreateTexture(const RHI_TextureDesc& desc) override;
        RHI_RenderPassHandle CreateRenderPass(const RHI_RenderPassDesc& desc) override;
        RHI_FramebufferHandle CreateFramebuffer(const RHI_FramebufferDesc& desc) override;

        RHI_ShaderHandle CreateShaderFromSPIRV(const void* data, size_t size) override;
        RHI_PipelineHandle CreateGraphicsPipeline(const RHI_PipelineDesc& desc) override;
        void UpdateBufferData(RHI_BufferHandle buffer, const void* data, size_t size) override;

        std::unique_ptr<IRHICommandList> CreateCommandList() override;
        void SubmitCommandList(IRHICommandList* cmdList) override;
        RHI_TextureHandle GetMainRenderTarget() const override;

        // 提供给 CommandList 的查询接口
        VkBuffer GetVkBuffer(RHI_BufferHandle h) const;
        VkFramebuffer GetVkFramebuffer(RHI_FramebufferHandle h) const;
        VkRenderPass GetVkRenderPass(RHI_RenderPassHandle h) const;
        VkPipeline GetVkPipeline(RHI_PipelineHandle h) const;

        VkDevice GetVkDevice() const
        {
            return m_device;
        }
        VkQueue GetGraphicsQueue() const
        {
            return m_graphicsQueue;
        }
        VkCommandPool GetCommandPool() const
        {
            return m_commandPool;
        }
        VkCommandPool GetFrameCommandPool() const
        {
            return m_frameCommandPools[m_currentFrame];
        }
        void* GetTextureRaw(RHI_TextureHandle) const override;

        VkPipelineLayout GetVkPipelineLayout(RHI_PipelineHandle h) const;

      private:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        uint32_t m_graphicsQueueFamily = 0;

        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        VkFormat m_swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
        VkExtent2D m_swapchainExtent{};

        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;

        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        VkCommandPool m_frameCommandPools[MAX_FRAMES_IN_FLIGHT] = {VK_NULL_HANDLE};
        std::vector<VkFramebuffer> m_swapchainFramebuffers;

        VkRenderPass m_swapchainRenderPass = VK_NULL_HANDLE;

        VkCommandBuffer m_commandBuffers[MAX_FRAMES_IN_FLIGHT] = {};
        VkSemaphore m_imageAvailable[MAX_FRAMES_IN_FLIGHT] = {};
        VkSemaphore m_renderFinished[MAX_FRAMES_IN_FLIGHT] = {};
        VkFence m_inFlightFences[MAX_FRAMES_IN_FLIGHT] = {};
        uint32_t m_currentFrame = 0;
        uint32_t m_currentImageIndex = 0;

        std::vector<VkCommandBuffer> m_frameCmdBuffers;

        std::atomic<uint32_t> m_nextRaw{1};

        struct BufferRecord
        {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            RHI_BufferDesc desc{};
        };
        struct TextureRecord
        {
            VkImage image = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            RHI_TextureDesc desc{};
        };
        struct RenderPassRecord
        {
            VkRenderPass renderPass = VK_NULL_HANDLE;
            RHI_RenderPassDesc desc{};
        };
        struct FramebufferRecord
        {
            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            RHI_FramebufferDesc desc{};
        };
        struct ShaderRecord
        {
            VkShaderModule module = VK_NULL_HANDLE;
        };
        struct PipelineRecord
        {
            VkPipeline pipeline = VK_NULL_HANDLE;
            VkPipelineLayout layout = VK_NULL_HANDLE;
        };

        std::unordered_map<uint32_t, BufferRecord> m_buffers;
        std::unordered_map<uint32_t, TextureRecord> m_textures;
        std::unordered_map<uint32_t, RenderPassRecord> m_renderPasses;
        std::unordered_map<uint32_t, FramebufferRecord> m_framebuffers;
        std::unordered_map<uint32_t, ShaderRecord> m_shaders;
        std::unordered_map<uint32_t, PipelineRecord> m_pipelines;

        RHI_TextureHandle m_mainRenderTarget;

        void CreateInstance(bool enableValidation);
        void CreateSurface(void* windowHandle);
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapchain(uint32_t width, uint32_t height);
        void CreateSwapchainImageViews();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateSwapchainRenderPass();
        void CreateSwapchainFramebuffers();

        uint32_t AllocateHandleRaw()
        {
            return m_nextRaw++;
        }

        void AllocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags properties, VkDeviceMemory& outMemory);
        void AllocateImageMemory(VkImage image, VkMemoryPropertyFlags properties, VkDeviceMemory& outMemory);
    };
} // namespace ChikaEngine::Render