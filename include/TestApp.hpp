#pragma once

#include "Appilcation.hpp"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace VKJourney::App
{
    constexpr uint32_t WIDTH = 800;
    constexpr uint32_t HEIGHT = 600;

    class TestApp : public IApplication<TestApp>
    {
      public:
        void Init();
        void Run();
        void Cleanup();

      private:
        void InitVulkan();
        void DrawFrame();
        void CleanupVulkan();

        void createInstance();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffer();
        void createSyncObjects();

        VkShaderModule createShaderModule(const std::vector<char>& code);

      private:
        GLFWwindow* m_window{nullptr};

        VkInstance m_instance;
        VkSurfaceKHR m_surface;

        VkPhysicalDevice m_physicalDevice;
        VkDevice m_device;
        VkQueue m_graphicQueue;
        VkQueue m_presentQueue;

        VkSwapchainKHR m_swapChain;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImage> m_swapChainImages;
        std::vector<VkImageView> m_swapChainImageViews;

        VkRenderPass m_renderPass;
        VkPipelineLayout m_pipelineLayout;
        VkPipeline m_graphicsPipeline;

        std::vector<VkFramebuffer> m_swapChainFramebuffers;

        VkCommandPool m_commandPool;
        VkCommandBuffer m_commandBuffer;

        VkSemaphore m_imageAvaliableSemaphore;
        VkSemaphore m_renderFinishedSemaphore;
        VkFence m_inFlightFence;
    };
} // namespace VKJourney::App