#include "TestApp.hpp"
#include <GLFW/glfw3.h>

namespace VKJourney::App
{

    void TestApp::Init()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanJourney", nullptr, nullptr);
    }

    void TestApp::Run()
    {
        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
        }
    }

    void TestApp::Cleanup()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
} // namespace VKJourney::App