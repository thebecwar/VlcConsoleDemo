// VlcConsoleDemo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "VlcPlayer.h"

#include <iostream>

// Compiler trick: Get the compiler to keep a copy of our instance handle.
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

// Global State Variables
ATOM playerWindowClassAtom = INVALID_ATOM;
HWND hPlayerWindow = NULL; // Handle for the player window we'll create.

// Forward Declarations - See the implementations below for info
void CreatePlayerWindow();
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SetPlayerItem(HWND owner, VlcPlayer* playerInstance);

int main()
{
    CreatePlayerWindow();
    if (playerWindowClassAtom == INVALID_ATOM)
    {
        int err = GetLastError();
        return err;
    }
    if (hPlayerWindow == NULL)
    {
        int err = GetLastError();
        return err;
    }

    ShowWindow(hPlayerWindow, SW_SHOW);

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

void CreatePlayerWindow()
{
    // Register the window class
    WNDCLASSEXW cls = { 0 }; // the '= { 0 }' initializes the struct to zero.
    cls.cbSize = sizeof(WNDCLASSEXW);
    cls.cbClsExtra = 0;
    cls.cbWndExtra = sizeof(VlcPlayer*); // Add an extra bit of data to the window, so we can stash the player pointer in there
    cls.hbrBackground = (HBRUSH)BLACK_BRUSH;
    cls.hCursor = NULL;
    cls.hIcon = NULL;
    cls.hIconSm = NULL;
    cls.hInstance = HINST_THISCOMPONENT;
    cls.lpfnWndProc = WndProc;
    cls.lpszClassName = L"VlcPlayerDemoClass";
    cls.lpszMenuName = NULL;
    cls.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    playerWindowClassAtom = RegisterClassExW(&cls);
    if (playerWindowClassAtom == INVALID_ATOM) return;

    hPlayerWindow = CreateWindowExW(0,                                  // Extended Style
                                    L"VlcPlayerDemoClass",              // Class Name
                                    L"VlcPlayerWindow",                 // Window Name
                                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,   // Window Style
                                    0,                                  // Initial X Position
                                    0,                                  // Initial Y Position
                                    300,                                // Initial Width
                                    300,                                // Initial Height
                                    NULL,                               // Parent Window
                                    NULL,                               // Menu Handle
                                    HINST_THISCOMPONENT,                // Instance Handle
                                    NULL);                              // Optional Parameter

}

// Window Procedure -
// Windows uses a message passing metaphor for window events. Different events create messages 
// from windows, and we can handle those events in this method.
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    VlcPlayer* playerInstance;
    switch (uMsg)
    {
    case WM_CREATE: // Window Created
        playerInstance = new VlcPlayer(hWnd); // Create the player
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG)playerInstance); // Store the player in the window's user data
        return (LRESULT)0; // Allow creation of the window
        break;
    case WM_DESTROY:
        // When the window is closed, tell the message loop to quit
        PostQuitMessage(0); 
        break;
    case WM_MEDIACOMPLETECALLBACK: // Media Player Completed Event
    case WM_LBUTTONDBLCLK: // Left Double Click Event
        playerInstance = (VlcPlayer*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        SetPlayerItem(hWnd, playerInstance);
        return (LRESULT)0;
        break;
    case WM_CHAR:
        playerInstance = (VlcPlayer*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        if ((wchar_t)wParam == L' ')
        {
            if (playerInstance->GetFilterState() == PlayerState::Running)
            {
                playerInstance->Pause();
            }
            else if (playerInstance->GetFilterState() == PlayerState::Paused)
            {
                playerInstance->Play();
            }
            return (LRESULT)0;
        }
        else if ((wchar_t)wParam == L'm')
        {
            playerInstance->SetMute(!playerInstance->GetMute());
            return (LRESULT)0;
        }

        break;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void SetPlayerItem(HWND owner, VlcPlayer* playerInstance)
{
    if (playerInstance != NULL)
    {
        wchar_t szFilename[MAX_PATH];
        wchar_t szFileTitle[MAX_PATH];
        ZeroMemory(szFilename, MAX_PATH);
        ZeroMemory(szFileTitle, MAX_PATH);

        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner = owner;
        ofn.hInstance = HINST_THISCOMPONENT;
        ofn.lpstrFilter = L"All Files\0*.*\0\0";
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 0;
        ofn.lpstrFile = szFilename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFileTitle = szFileTitle;
        ofn.nMaxFileTitle = MAX_PATH;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = L"Open Media File";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
        
        BOOL ok = GetOpenFileNameW(&ofn);
        if (ok)
        {
            playerInstance->SetFilename(ofn.lpstrFile);
            playerInstance->Play();
        }
        else
        {
            DWORD extErr = CommDlgExtendedError();
            std::wcout << L"Open File Dialog Error: " << std::hex << extErr << std::endl;
            
        }
    }
}