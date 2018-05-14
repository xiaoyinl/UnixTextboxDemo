// Licensed under MIT
/* Source code for the blog post "Let's Figure out how Notepad Supports Unix Line Endings"
   Part I: https://medium.com/bugbountywriteup/lets-figure-out-how-notepad-supports-unix-line-endings-part-i-26d54b29cf93
   Part II: https://medium.com/bugbountywriteup/lets-figure-out-how-notepad-supports-unix-line-endings-part-ii-2755c001611c
*/

#include <Windows.h>
#include <Commctrl.h>  // for InitCommonControlsEx
#include <string>
#include <algorithm>
#pragma comment(lib, "Comctl32.lib")

// add manifest to enable Comctl32 version2. Otherwise User32.dll controls are used.
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
HWND g_hwndMain = nullptr;    // top window
HWND g_hwndEdit = nullptr;    // Edit: main textarea
HWND g_hwndBtnGet = nullptr;  // button: get line ending type
HWND g_hwndBtnSet = nullptr;  // button: set line ending type
HWND g_hwndBtnText = nullptr; // button: get text in Edit control
HWND g_hwndBtnLF = nullptr;   // button: insert \n to Edit
HWND g_hwndBtnCRLF = nullptr; // button: insert \r\n to Edit
HINSTANCE g_hInstance = nullptr;

LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CreateControls(HWND);
void HandleControlCommands(UINT code, HWND hwnd);
void ShowInfoMessage(const std::wstring& wsMsg);
bool appendTextToEdit(HWND hEdit, const wchar_t *str);

#define MSGID_ENABLE_EOL 0x150A
#define MSGID_SETEOLTYPE 0x150C
#define MSGID_GETEOLTYPE 0x150D

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    const static wchar_t *swMainClassName = L"main";
    const static wchar_t *swTitle = L"Support of Unix Line Ending in EDIT Control";

    g_hInstance = hInstance;

    WNDCLASSEXW wc{ sizeof(WNDCLASSEXW), 0, WndProc, 0, 0, hInstance, LoadIconA(nullptr, IDI_APPLICATION),
        LoadCursorA(nullptr, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), nullptr, swMainClassName,
        LoadIconA(nullptr, IDI_APPLICATION) };

    if (!RegisterClassExW(&wc)) return 1;

    g_hwndMain = CreateWindowExW(WS_EX_CLIENTEDGE, swMainClassName, swTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 400, NULL, NULL, hInstance, nullptr);
    if (!g_hwndMain) return 2;

    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);

    MSG Msg;
    while (GetMessageW(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }
    return (int)Msg.wParam;
}
LRESULT __stdcall WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateControls(hwnd);
        break;
    case WM_SIZE:
        {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            if (g_hwndEdit)
                SetWindowPos(g_hwndEdit, NULL, 0, 0, rcClient.right, rcClient.bottom - 50, SWP_NOZORDER);
            HWND arr_hwndBtn[] = {g_hwndBtnGet, g_hwndBtnSet, g_hwndBtnText , g_hwndBtnLF , g_hwndBtnCRLF };
            for (size_t i = 0; i < _countof(arr_hwndBtn); i++)
            {
                if (arr_hwndBtn[i])
                {
                    SetWindowPos(arr_hwndBtn[i], NULL, rcClient.right * i / _countof(arr_hwndBtn), rcClient.bottom - 50,
                        rcClient.right / _countof(arr_hwndBtn), 50, SWP_NOZORDER);
                }
            }
        }
        break;
    case WM_COMMAND:
        HandleControlCommands(HIWORD(wParam), (HWND)lParam);
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}
bool CreateControls(HWND hwnd)
{
    INITCOMMONCONTROLSEX st{sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
    if (InitCommonControlsEx(&st)) {
        g_hwndEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 10, 10, hwnd, NULL, g_hInstance, nullptr);
        
        // The following message ID is found by viewing the call stack when Notepad reads registry:
        // corresponds to Notepad's fWindowsOnlyEOL option. Set wParam and lParam to 3 to disable fWindowsOnlyEOL
        SendMessageW(g_hwndEdit, MSGID_ENABLE_EOL, 3, 3);
        // corresponds to Notepad's fPasteOriginalEOL option. Set wParam and lParam to 4 to disable fPasteOriginalEOL
        SendMessageW(g_hwndEdit, MSGID_ENABLE_EOL, 4, 4);
        SetWindowTextW(g_hwndEdit, L"First line ends with \\n.\nSecond line ends with \\r\\n.\r\nThird line ends with \\r.\rLast line doesn't have a line ending.");
        
        g_hwndBtnGet = CreateWindowExW(WS_EX_CLIENTEDGE, L"BUTTON", L"Line Ending Type",
            WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, NULL,
            g_hInstance, nullptr);

        g_hwndBtnSet = CreateWindowExW(WS_EX_CLIENTEDGE, L"BUTTON", L"Set Line Ending",
            WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, NULL,
            g_hInstance, nullptr);

        g_hwndBtnText = CreateWindowExW(WS_EX_CLIENTEDGE, L"BUTTON", L"Get Text",
            WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, NULL,
            g_hInstance, nullptr);
    
        g_hwndBtnLF = CreateWindowExW(WS_EX_CLIENTEDGE, L"BUTTON", L"Insert \\n",
            WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, NULL,
            g_hInstance, nullptr);

        g_hwndBtnCRLF = CreateWindowExW(WS_EX_CLIENTEDGE, L"BUTTON", L"Insert \\r\\n",
            WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, NULL,
            g_hInstance, nullptr);
    }
    return (g_hwndEdit && g_hwndBtnGet && g_hwndBtnSet && g_hwndBtnText && g_hwndBtnLF && g_hwndBtnCRLF);
}
void HandleControlCommands(UINT code, HWND hwnd)
{
    if (!hwnd) return;  // not Control
    
    if (code == BN_CLICKED)
    {
        if (hwnd == g_hwndBtnGet)
        {
            HLOCAL hOldMem = (HLOCAL)SendMessageW(g_hwndEdit, EM_GETHANDLE, 0, 0);
            HLOCAL hNewMem = LocalReAlloc(hOldMem, 1024, LMEM_MOVEABLE);
            SendMessageW(g_hwndEdit, EM_SETHANDLE, (WPARAM)hNewMem, 0);

            LRESULT type = SendMessageW(g_hwndEdit, MSGID_GETEOLTYPE, 0, 0);
            switch (type)
            {
            case 0:
                ShowInfoMessage(L"Retrieving line ending type is not supported.");
                break;
            case 1:
                ShowInfoMessage(L"Windows CRLF");
                break;
            case 2:
                ShowInfoMessage(L"Macintosh CR");
                break;
            case 3:
                ShowInfoMessage(L"Unix LF");
                break;
            default:
                ShowInfoMessage(L"Unknown line ending type: " + std::to_wstring(type));
            }
        }
        else if (hwnd == g_hwndBtnSet)
        {
            int option = MessageBoxW(g_hwndMain, L"Abort: Windows CRLF\r\nRetry: Macintosh CR\r\nIgnore: Unix LF",
                L"Select Line Ending Type", MB_ABORTRETRYIGNORE | MB_ICONQUESTION);
            int typecode = 0;
            switch (option)
            {
            case IDABORT:
                typecode = 1;   // Windows
                break;
            case IDRETRY:
                typecode = 2;   // Macintosh
                break;
            case IDIGNORE:
                typecode = 3;   // Unix
                break;
            default:
                break;
            }
            SendMessageW(g_hwndEdit, MSGID_SETEOLTYPE, typecode, 0);
        }
        else if (hwnd == g_hwndBtnText)
        {
            int len = GetWindowTextLengthW(g_hwndEdit);   // doesn't include null terminator
            wchar_t *textBuf = new wchar_t[len + 1];
            GetWindowTextW(g_hwndEdit, textBuf, len + 1);
            std::wstring wsText(textBuf);
            delete[] textBuf;
            int index;
            while ((index = wsText.find(L"\r")) != std::wstring::npos)
            {
                wsText.replace(index, 1, L"<CR>");
            }
            while ((index = wsText.find(L"\n")) != std::wstring::npos)
            {
                wsText.replace(index, 1, L"<LF>");
            }
            ShowInfoMessage(wsText);
        }
        else if (hwnd == g_hwndBtnLF)
        {
            appendTextToEdit(g_hwndEdit, L"\n");
        }
        else if (hwnd == g_hwndBtnCRLF)
        {
            appendTextToEdit(g_hwndEdit, L"\r\n");
        }
    }
}
void ShowInfoMessage(const std::wstring& wsMsg)
{
    MessageBoxW(g_hwndMain, wsMsg.c_str(), L"Info", MB_OK);
}
bool appendTextToEdit(HWND hEdit, const wchar_t *str)
{
    int index = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
    SendMessageW(hEdit, EM_REPLACESEL, NULL, (LPARAM)str);
    return true;
}
