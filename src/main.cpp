// main.cpp
#include "TestApp.hpp"
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

int main()
{
    VKJourney::App::TestApp testApp;
    testApp.CheckApplication();
    testApp.RunApp();
}