// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h> // For Windows API
#include <iostream> // For std::cout
#include <cstdio> // For freopen_s
#include "MinHook.h" // MinHook library

// Link MinHook library
#pragma comment(lib, "MinHook.x86.lib")

// --- 1. DECLARE ---
// Prototype of SDL_GetTicks (Return ms - Uint32)
typedef uint32_t(*SDL_GetTicks_t)(void);
SDL_GetTicks_t oSDL_GetTicks = nullptr;

// Global for speed control
float g_SpeedMultiplier = 1.0f; // 1.0 = Normal speed
uint32_t g_BaseTick = 0;        // Emulated game tick
uint32_t g_LastRealTick = 0;    // Real tick at last call

// --- 2. HOOK FUNCTION ---
uint32_t Hooked_SDL_GetTicks(void) {
	// Get current real tick
    uint32_t currentRealTick = oSDL_GetTicks();

	// Initialize on first call (when just injected)
	if (g_LastRealTick == 0) { // First call
		g_LastRealTick = currentRealTick; // Set last real tick 
		g_BaseTick = currentRealTick; // Initialize base tick
		return currentRealTick; // Return real tick on first call
    }

	// If no intialization on first call, the game time will jump unexpectedly because of large delta, causing glitches (sốc) or crashes.

	// Calculate time delta since last call
    uint32_t delta = currentRealTick - g_LastRealTick;

	// Apply speed multiplier to delta and update base tick
    g_BaseTick += (uint32_t)(delta * g_SpeedMultiplier);

	// Update last real tick
    g_LastRealTick = currentRealTick;

    return g_BaseTick;
}

// --- 3. MAIN THREAD ---
DWORD WINAPI MainThread(LPVOID lpReserved) {
	AllocConsole(); // Create a console for output debug
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "[+] SpeedHack DLL Injected!" << std::endl;

	// Initialize MinHook
    if (MH_Initialize() != MH_OK) 
        return 1;

	// Hook SDL_GetTicks from SDL2.dll
    if (MH_CreateHookApi(L"SDL2.dll", "SDL_GetTicks", &Hooked_SDL_GetTicks, (LPVOID*)&oSDL_GetTicks) != MH_OK) {
        std::cout << "[-] Hook SDL_GetTicks Failed! Check DLL name." << std::endl;
        return 1;
    }

	// Enable the hook
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
        return 1;

    std::cout << "[+] Hook Ready!" << std::endl;
    std::cout << "[F1] Speed x1 (Normal)" << std::endl;
    std::cout << "[F2] Speed x2 (Fast)" << std::endl;
    std::cout << "[F3] Speed x5 (Super Fast)" << std::endl;
    std::cout << "[F4] Speed x0.5 (Slow Motion)" << std::endl;

    while (true) {
		// Shortcuts to change speed
		if (GetAsyncKeyState(VK_F1) & 1) {  // F1: Normal speed
            g_SpeedMultiplier = 1.0f;
            std::cout << "[*] Speed: Normal (x1.0)" << std::endl;
        }
		if (GetAsyncKeyState(VK_F2) & 1) { // F2: Fast speed (x2)
            g_SpeedMultiplier = 2.0f;
            std::cout << "[*] Speed: Fast (x2.0)" << std::endl;
        }
		if (GetAsyncKeyState(VK_F3) & 1) { // F3: Super speed (x5)
            g_SpeedMultiplier = 5.0f;
            std::cout << "[*] Speed: Super (x5.0)" << std::endl;
        }
		if (GetAsyncKeyState(VK_F4) & 1) { // F4: Slow motion (x0.5)
            g_SpeedMultiplier = 0.5f;
            std::cout << "[*] Speed: Slow (x0.5)" << std::endl;
        }

		if (GetAsyncKeyState(VK_END) & 1) // END: Exit and unhook
            break;

		Sleep(10); // Sleep to reduce CPU usage
    }

	MH_DisableHook(MH_ALL_HOOKS); // Disable all hooks
	MH_Uninitialize(); // Uninitialize MinHook
	FreeConsole(); // Free the console
	FreeLibraryAndExitThread((HMODULE)lpReserved, 0); // Exit the thread and unload DLL
    return 0;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID l) {
	if (r == DLL_PROCESS_ATTACH) { // When the DLL is loaded
		DisableThreadLibraryCalls(h); // Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications
		CreateThread(0, 0, MainThread, h, 0, 0); // Create main thread
    }
    return TRUE;
}