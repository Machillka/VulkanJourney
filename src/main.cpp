// // main.cpp
// #include "TestApp.hpp"
// #include <GLFW/glfw3.h>

// #include <vulkan/vulkan.hpp>
// #include <vulkan/vulkan_raii.hpp>

// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE

// #include <glm/mat4x4.hpp>
// #include <glm/vec4.hpp>

// int main()
// {
//     VKJourney::App::TestApp testApp;
//     testApp.CheckApplication();
//     testApp.RunApp();
// }
// test/main.cpp

#include "ChikaEngine/Renderer.hpp"
#include "ChikaEngine/math/mat4.h"
#include "ChikaEngine/rhi/Mock/MockResourceView.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

int main()
{
    using namespace ChikaEngine::Render;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    const int width = 1280;
    const int height = 720;
    auto window = glfwCreateWindow(width, height, "VulkanJourney", nullptr, nullptr);

    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    try
    {
        Test::MockResourceView resourceView;

        Renderer& renderer = Renderer::Instance();
        RendererCreateInfo ci{};
        ci.windowHandle = window;
        ci.width = width;
        ci.height = height;
        ci.backend = RHIBackendTypes::Vulkan;
        ci.resourceView = &resourceView;

        renderer.Init(ci);

        CameraData cam{};
        cam.view = ChikaEngine::Math::Mat4::Identity();
        cam.proj = ChikaEngine::Math::Mat4::Identity();
        renderer.SetCamera(cam);

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            renderer.BeginFrame();
            renderer.EndFrame();
        }

        renderer.Shutdown();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
