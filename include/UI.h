#pragma once
#include "Configuration.h"
#include "SKSEMenuFramework.h"

namespace UI {
    void Register();

    namespace ConsoleCommander {
        void __stdcall Render();
        void __stdcall RenderAddCommandWindow();
        inline MENU_WINDOW AddCommandWindow;
    }
}