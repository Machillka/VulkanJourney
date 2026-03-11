#pragma once

namespace VKJourney
{
    template <class T>
    concept ApplicationImpl = requires(T c) {
        c.Init();
        c.Run();
        c.Cleanup();
    };

    template <class Derived> class IApplication
    {
      public:
        void CheckApplication()
        {
            static_assert(ApplicationImpl<Derived>, "Derived class Miss some functions");
        }
        void RunApp()
        {
            static_cast<Derived*>(this)->Init();
            static_cast<Derived*>(this)->Run();
            static_cast<Derived*>(this)->Cleanup();
        }
    };

} // namespace VKJourney