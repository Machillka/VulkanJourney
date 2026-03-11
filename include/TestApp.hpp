#pragma once

#include "Appilcation.hpp"
#include <cstdint>

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
        GLFWwindow* m_window{nullptr};

        
    };
} // namespace VKJourney::App