#include <Windows.h>
#include <Commdlg.h>
#include <tlhelp32.h>
#include <iostream>
#include <ctime>
#include "../include/discord_rpc.h"

DWORD GetProcId(const wchar_t* procName);
int performInjection(DWORD procId, const wchar_t* dllPath);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ShowFileBrowseDialog(HWND hwnd);

wchar_t g_szFilePath[MAX_PATH] = L"";
WCHAR g_szProcessName[MAX_PATH] = L"Minecraft.Windows.exe";
HWND g_hStatusText = nullptr;
HWND g_hPlayingText = nullptr;

DWORD GetProcId(const wchar_t* procName)
{
    DWORD procId = 0;
    char narrowProcName[MAX_PATH];

    if (WideCharToMultiByte(CP_ACP, 0, procName, -1, narrowProcName, MAX_PATH, NULL, NULL))
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32 procEntry;
            procEntry.dwSize = sizeof(procEntry);

            if (Process32First(hSnap, &procEntry))
            {
                do
                {
                    if (!_stricmp(procEntry.szExeFile, narrowProcName))
                    {
                        procId = procEntry.th32ProcessID;
                        break;
                    }
                } while (Process32Next(hSnap, &procEntry));
            }

            CloseHandle(hSnap);
        }
    }

    return procId;
}

int performInjection(DWORD procId, const wchar_t* dllPath)
{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);

    if (hProc && hProc != INVALID_HANDLE_VALUE)
    {
        void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        WriteProcessMemory(hProc, loc, dllPath, wcslen(dllPath) * 2 + 2, 0);

        HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, loc, 0, 0);

        if (hThread)
        {
            CloseHandle(hThread);
        }
    }

    if (hProc)
    {
        CloseHandle(hProc);
    }

    return 0;
}

void UpdateDiscordPresence(const wchar_t* dllFileName)
{
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.largeImageKey = "invlogo";
    discordPresence.state = "In-Game";

    if (dllFileName && *dllFileName)
    {
        wchar_t playingText[MAX_PATH + 10] = L"Playing: ";
        wcscat_s(playingText, dllFileName);
        UpdateDiscordPresence(dllFileName);
    }
    else
    {
        discordPresence.details = "No DLL selected!";
    }

    Discord_UpdatePresence(&discordPresence);
    UpdateDiscordPresence(nullptr);
}

void ShowFileBrowseDialog(HWND hwnd)
{
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"DLL Files (*.dll)\0*.dll\0All Files\0*.*\0";
    ofn.lpstrFile = g_szFilePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;

    if (GetOpenFileNameW(&ofn))
    {
        SetWindowTextW(GetDlgItem(hwnd, 3), g_szFilePath);

        WCHAR* dllFileName = wcsrchr(g_szFilePath, L'\\');
        if (dllFileName)
        {
            dllFileName++;
            WCHAR playingText[MAX_PATH + 10] = L"Playing: ";
            wcscat_s(playingText, dllFileName);
            SetWindowTextW(g_hPlayingText, playingText);
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFont = nullptr;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Arial");

        HWND hInjectButton = CreateWindowW(
            L"BUTTON",
            L"Inject!",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 10, 100, 30,
            hwnd,
            (HMENU)1,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            nullptr
        );

        HWND hProcessNameInput = CreateWindowW(
            L"EDIT",
            g_szProcessName,
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            120, 10, 205, 30,
            hwnd,
            (HMENU)2,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            nullptr
        );

        HWND hBrowseButton = CreateWindowW(
            L"BUTTON",
            L"Browse",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 50, 70, 30,
            hwnd,
            (HMENU)4,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            nullptr
        );

        g_hStatusText = CreateWindowW(
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            90, 50, 235, 30,
            hwnd,
            nullptr,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            nullptr
        );

        g_hPlayingText = CreateWindowW(
            L"STATIC",
            L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            10, 90, 320, 30, 
            hwnd,
            nullptr,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            nullptr
        );

        SetWindowTextW(g_hPlayingText, L"No DLL selected!");

        HBRUSH grayBrush = CreateSolidBrush(RGB(240, 240, 240));
        SetClassLongPtr(g_hPlayingText, GCLP_HBRBACKGROUND, (LONG_PTR)grayBrush);

        SendMessageW(hInjectButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hProcessNameInput, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hBrowseButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(g_hStatusText, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(g_hPlayingText, WM_SETFONT, (WPARAM)hFont, TRUE);

        break;
    }
    case WM_GETDLGCODE:
    {
        HWND hProcessNameInput = GetDlgItem(hwnd, 2);

        if (hProcessNameInput == (HWND)wParam && (lParam & DLGC_WANTALLKEYS))
        {
            return DLGC_WANTALLKEYS;
        }
        break;
    }
    case WM_COMMAND:
    {   
        if (LOWORD(wParam) == 1)
        {
            GetDlgItemTextW(hwnd, 2, g_szProcessName, MAX_PATH);
            DWORD procId = GetProcId(g_szProcessName);

            if (procId != 0)
            {
                performInjection(procId, g_szFilePath);
                SetWindowTextW(g_hStatusText, L"Injection successful!");
            }
            else
            {
                SetWindowTextW(g_hStatusText, L"Process not found!");
            }
        }
        else if (LOWORD(wParam) == 4)
        {
            ShowFileBrowseDialog(hwnd);

            WCHAR playingText[MAX_PATH + 10] = L"Playing: ";
            wcscat_s(playingText, g_szFilePath);
            SetWindowTextW(g_hPlayingText, playingText);
        }
        break;
    }
    case WM_DESTROY:
    {
        if (hFont)
        {
            DeleteObject(hFont);
            hFont = nullptr;
        }

        Discord_Shutdown();

        PostQuitMessage(0);
        break;
    }
    case WM_CHAR:
    {
        HWND hProcessNameInput = GetDlgItem(hwnd, 2);

        if (GetKeyState(VK_CONTROL) < 0 && wParam == 'A')
        {
            SendMessageW(hProcessNameInput, EM_SETSEL, 0, -1);
            return 0;
        }

        if (GetKeyState(VK_CONTROL) < 0 && wParam == 'C')
        {
            SendMessageW(hProcessNameInput, WM_COPY, 0, 0);
            return 0;
        }
        break;
    }
    default:
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"ChyvesInjector";

    HICON hIcon = LoadIcon(NULL, IDI_APPLICATION);
    HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = hIcon;
    wc.hCursor = hCursor;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Chyves Injector",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX) & ~(WS_SIZEBOX),
        CW_USEDEFAULT, CW_USEDEFAULT, 360, 170,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hwnd == nullptr)
    {
        return 0;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);

    MSG msg;

    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize("1133864646223343676", &handlers, 1, nullptr);

    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.largeImageKey = "invlogo";
    discordPresence.startTimestamp = std::time(0);
    discordPresence.details = "No DLL selected!";
    Discord_UpdatePresence(&discordPresence);

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
