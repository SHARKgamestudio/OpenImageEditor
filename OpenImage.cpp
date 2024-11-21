
#pragma region External Dependencies

#include <vector>
#include <string>
#include <codecvt>
#include <iostream>
#include <objidl.h>
#include <Windows.h>
#include <gdiplus.h>
#include <commctrl.h>

#pragma endregion

#pragma region Local Dependencies

#include "framework.h"
#include "OpenImage.h"

#pragma endregion

#define MAX_LOADSTRING 64

#define DEFAULT_WIDTH  960
#define DEFAULT_HEIGHT 540

#pragma region Macro

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma endregion

#pragma region Namespaces

using namespace Gdiplus;
using namespace std;

#pragma endregion

// Global Variables
HINSTANCE hInst;

HWND hSlider;
HWND hEncodeButton, hEncodeEdit, hDecodeButton, hDecodeEdit;

wstring current_path;
float zoomFactor = 1.0f;

WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

FileExplorer* fileExplorer;

// Function Prototypes
ATOM RegisterAppClass(HINSTANCE hInstance);
BOOL InitializeApp(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void DrawImageInBoxWithZoom(HDC hdc, int boxX, int boxY, int boxWidth, int boxHeight, float zoom) {
    Graphics graphics(hdc);

    Image image(current_path.c_str());

    UINT imgWidth = image.GetWidth();
    UINT imgHeight = image.GetHeight();

    int scaledWidth = static_cast<int>(imgWidth * zoom);
    int scaledHeight = static_cast<int>(imgHeight * zoom);

    int offsetX = boxX - (scaledWidth - boxWidth) / 2;
    int offsetY = boxY - (scaledHeight - boxHeight) / 2;

    Region clipRegion(Rect(boxX, boxY, boxWidth, boxHeight));
    graphics.SetClip(&clipRegion);

    Rectangle(hdc, boxX, boxY, boxX + boxWidth, boxY + boxHeight);

    graphics.DrawImage(&image, Rect(offsetX, offsetY, scaledWidth, scaledHeight));
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_OPENIMAGE, szWindowClass, MAX_LOADSTRING);
    RegisterAppClass(hInstance);

    if (!InitializeApp(hInstance, nCmdShow)) return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OPENIMAGE));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

ATOM RegisterAppClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BIG));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_OPENIMAGE);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitializeApp(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_MORE_ABOUT: {
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDialogProc);
            return 0;
        }
        case IDM_FILES_EXIT: {
            DestroyWindow(hWnd);
            return 0;
        }
        case IDM_FILES_OPEN: {
			fileExplorer = new FileExplorer(hWnd, L"PNG Files\0*.PNG\0JPEG Files\0*.JPG\0");
            if (fileExplorer->OpenFile()) {
				current_path = fileExplorer->GetPath();

                RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE);
                InvalidateRect(hWnd, NULL, TRUE);

                EnableWindow(hEncodeEdit, TRUE); EnableWindow(hEncodeButton, TRUE);
                EnableWindow(hSlider, TRUE); EnableWindow(hDecodeButton, TRUE);

                SetWindowText(hEncodeEdit, L"type something to encrypt.");
                SetWindowText(hDecodeEdit, L"");
            }

            return 0;
        }
        case ID_ENCODE_BUTTON: {
            wchar_t buffer[256];
            GetWindowText(hEncodeEdit, buffer, sizeof(buffer));

            fileExplorer = new FileExplorer(hWnd, L"PNG Files\0*.png\0All Files\0*.*\0");
			if (fileExplorer->SaveFile()) {
                Bitmap* encodedImage = nullptr;
                if (EncodeMessage(current_path, buffer, encodedImage)) {
                    SaveImage(encodedImage, fileExplorer->GetPath());
                    delete encodedImage;
                }
			}

            return 0;
        }
        case ID_DECODE_BUTTON: {
            wstring message = DecodeMessage(current_path).c_str();
			SetWindowText(hDecodeEdit, message.c_str());
            return 0;
        }
        default: {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        }
        return 0;
    }
    case WM_CREATE: {
        // Create the slider (trackbar) control
        hSlider = CreateWindowEx(
            0, TRACKBAR_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_AUTOTICKS,
            0, 0, 100, 100, // Initial size (updated in WM_SIZE)
            hWnd, (HMENU)1, hInst, NULL);

        // Configure slider range and initial position
        SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(1, 100)); // Range: 1 to 100
        SendMessage(hSlider, TBM_SETPOS, TRUE, 50);                   // Initial position: 50

        hEncodeButton = CreateWindow(
            L"BUTTON",              // Predefined class for buttons
            L"Encrypt",            // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
            550, 50, 100, 30,       // x, y, width, height
            hWnd,                  // Parent window handle
            (HMENU)ID_ENCODE_BUTTON,      // Button identifier
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL                   // Pointer to additional data
        );

        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
        HFONT hFont = ::CreateFontIndirect(&ncm.lfMessageFont);
        ::SendMessage(hEncodeButton, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

        hDecodeButton = CreateWindow(
            L"BUTTON",              // Predefined class for buttons
            L"Decrypt",            // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
            550, 100, 100, 30,       // x, y, width, height
            hWnd,                  // Parent window handle
            (HMENU)ID_DECODE_BUTTON,      // Button identifier
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL                   // Pointer to additional data
        );

        NONCLIENTMETRICS ncm2;
        ncm2.cbSize = sizeof(NONCLIENTMETRICS);
        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm2, 0);
        HFONT hFont2 = ::CreateFontIndirect(&ncm2.lfMessageFont);
        ::SendMessage(hDecodeButton, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

        hEncodeEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,  // Style: adding a border effect
            L"EDIT",            // Class name for the edit control
            L"please open an image first.",                // Initial text in the edit control
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, // Styles (visible, child, etc.)
            10, 10,            // Position (x, y)
            200, 25,           // Size (width, height)
            hWnd,              // Parent window
            (HMENU)ID_EDIT,          // Control ID
            (HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE),
            NULL
        );             // Additional creation data

        NONCLIENTMETRICS ncm3;
        ncm3.cbSize = sizeof(NONCLIENTMETRICS);
        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm3, 0);
        HFONT hFont3 = ::CreateFontIndirect(&ncm3.lfMessageFont);
        ::SendMessage(hEncodeEdit, WM_SETFONT, (WPARAM)hFont3, MAKELPARAM(TRUE, 0));

        hDecodeEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,  // Style: adding a border effect
            L"EDIT",            // Class name for the edit control
            L"please open an image first.",                // Initial text in the edit control
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, // Styles (visible, child, etc.)
            10, 10,            // Position (x, y)
            200, 25,           // Size (width, height)
            hWnd,              // Parent window
            (HMENU)ID_DECODE_EDIT,          // Control ID
            (HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE),
            NULL
        );             // Additional creation data

        NONCLIENTMETRICS ncm4;
        ncm4.cbSize = sizeof(NONCLIENTMETRICS);
        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm4, 0);
        HFONT hFont4 = ::CreateFontIndirect(&ncm4.lfMessageFont);
        ::SendMessage(hDecodeEdit, WM_SETFONT, (WPARAM)hFont4, MAKELPARAM(TRUE, 0));

        EnableWindow(hEncodeEdit, FALSE);
        EnableWindow(hDecodeEdit, FALSE);
        EnableWindow(hEncodeButton, FALSE);
        EnableWindow(hDecodeButton, FALSE);
        EnableWindow(hSlider, FALSE);

        return 0;
    }
    case WM_SIZE: {
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int windowWidth = clientRect.right - clientRect.left;
        int windowHeight = clientRect.bottom - clientRect.top;

        int boxWidth = static_cast<int>(windowWidth * 0.5);
        int boxHeight = static_cast<int>(windowHeight * 0.90);
        int boxX = 0;
        int boxY = (windowHeight - boxHeight) / 2;

        int sliderWidth = 25;
        int sliderHeight = boxHeight;
        int sliderX = boxWidth + ((windowWidth - boxWidth) / 16);
        int sliderY = boxY;

        int editWidth = (windowWidth - (boxWidth + ((windowWidth - boxWidth) / 16))) - 150;
        int editHeight = 30;
        int editX = boxWidth + 25 + ((windowWidth - boxWidth) / 16) + 8;
        int editY = boxY;

        int editDecodeWidth = (windowWidth - (boxWidth + ((windowWidth - boxWidth) / 16))) - 150;
        int editDecodeHeight = 30;
        int editDecodeX = boxWidth + 25 + ((windowWidth - boxWidth) / 16) + 8;
        int editDecodeY = boxY + 38;

        int encodeWidth = 100;
        int encodeHeight = 30;
        int encodeX = (boxWidth + ((windowWidth - boxWidth) / 16) + editWidth) + 41;
        int encodeY = boxY;

        int decodeWidth = 100;
        int decodeHeight = 30;
        int decodeX = (boxWidth + ((windowWidth - boxWidth) / 16) + editWidth) + 41;
        int decodeY = boxY + 38;

        SetWindowPos(hSlider, NULL, sliderX, sliderY, sliderWidth, sliderHeight, SWP_NOZORDER);

        SetWindowPos(hEncodeEdit, NULL, editX, editY, editWidth, editHeight, SWP_NOZORDER);
        SetWindowPos(hDecodeEdit, NULL, editDecodeX, editDecodeY, editDecodeWidth, editDecodeHeight, SWP_NOZORDER);

        SetWindowPos(hEncodeButton, NULL, encodeX, encodeY, encodeWidth, encodeHeight, SWP_NOZORDER);
        SetWindowPos(hDecodeButton, NULL, decodeX, decodeY, decodeWidth, decodeHeight, SWP_NOZORDER);
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }
    case WM_VSCROLL: {
        if ((HWND)lParam == hSlider) {
            int sliderPos = SendMessage(hSlider, TBM_GETPOS, 0, 0);
            zoomFactor = 1.0f + (sliderPos - 50) / 50.0f;
            InvalidateRect(hWnd, NULL, TRUE);
        }
        return 0;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 680;
        mmi->ptMinTrackSize.y = 380;
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Get the dimensions of the client area
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int windowWidth = clientRect.right - clientRect.left;
        int windowHeight = clientRect.bottom - clientRect.top;

        // Define the box size and position (example: centered box taking 50% width and 40% height)
        int boxWidth = static_cast<int>(windowWidth * 0.5);
        int boxHeight = static_cast<int>(windowHeight * 0.90);
        int boxX = (windowWidth - boxWidth) / 16; // Center horizontally
        int boxY = (windowHeight - boxHeight) / 2; // Center vertically

        // Call the function with the calculated box dimensions
        DrawImageInBoxWithZoom(hdc, boxX, boxY, boxWidth, boxHeight, zoomFactor);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    if (message == WM_INITDIALOG) return (INT_PTR)TRUE;
    if (message == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)) {
        EndDialog(hDlg, LOWORD(wParam));
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}