#include <iostream>
#include <Windows.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <memory>
#include "overlay/overlay.h"
#include "features/esp.h"
#include "sdk/offset_updater.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "winmm.lib")

std::unique_ptr<ActorLoopClass> ActorLoop = std::make_unique<ActorLoopClass>();
std::unique_ptr<Overlay> OverlayInstance = std::make_unique<Overlay>();

int main()
{
    timeBeginPeriod(1);
    SetConsoleTitleA("noc-external");

    std::cout << R"(
                               _                        _ 
                              | |                      | |
  _ __   ___   ___    _____  _| |_ ___ _ __ _ __   __ _| |
 | '_ \ / _ \ / __|  / _ \ \/ / __/ _ \ '__| '_ \ / _` | |
 | | | | (_) | (__  |  __/>  <| ||  __/ |  | | | | (_| | |
 |_| |_|\___/ \___|  \___/_/\_\\__\___|_|  |_| |_|\__,_|_|
                                                          
                                                          
                                        
    )" << std::endl;
    std::cout << "[+] Offsets credits to imtheo.lol" << std::endl;
    std::cout << "[+] Press Insert to open the menu" << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
   
    std::cout << "[*] Checking for offset updates..." << std::endl;
    if (OffsetUpdater::UpdateOffsets())
    {
        std::cout << "[+] Offsets updated successfully! :)" << std::endl;
    }
    else
    {
        std::cout << "[-] Failed to update offsets, using existing ones..." << std::endl;
    }
    std::cout << std::endl;
    
    if (!ActorLoop->Initialize())
    {
        std::cout << "Failed find Process :( - make sure Roblox is running" << std::endl;
        std::cin.get();
        return 1;
    }

    if (!OverlayInstance->Initialize())
    {
        std::cout << "Failed to initialize overlay - check DirectX support" << std::endl;
        std::cin.get();
        return 1;
    }
    
    OverlayInstance->SetClickThrough(true);
    
    // Set process priority to a lower level to ensure it doesn't starve the game or other background processes.
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    
    static bool menu_was_visible = false;

    while (OverlayInstance->IsRunning())
    {
        OverlayInstance->BeginFrame();
        
        // Single data gathering step to prevent lag
        ActorLoop->Tick();
        
        ActorLoop->RenderMenu();
        
        bool menu_visible = ActorLoop->IsMenuVisible();
        if (menu_visible != menu_was_visible)
        {
            OverlayInstance->SetClickThrough(!menu_visible);
            menu_was_visible = menu_visible;
            
            if (menu_visible)
            {
                HWND hwnd = OverlayInstance->GetWindowHandle();
                if (hwnd)
                {
                    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    SetForegroundWindow(hwnd);
                    BringWindowToTop(hwnd);
                }
            }
        }
        
        ActorLoop->UpdateAimbot();
        ActorLoop->UpdateMisc();
        ActorLoop->Render(OverlayInstance.get());
        
        OverlayInstance->EndFrame();
        
        // Balanced sleep: 15ms (~66 FPS) is perfect for an external without causing lag.
        Sleep(15);
    }
    
    timeEndPeriod(1);
    return 0;
}
