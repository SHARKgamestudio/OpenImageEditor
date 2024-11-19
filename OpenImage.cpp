// OpenImage.cpp : Defines the entry point for the application.

#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <string>

#include "framework.h"
#include "OpenImage.h"

#define MAX_LOADSTRING 100

#pragma comment (lib, "Gdiplus.lib")
#pragma comment(lib, "Comctl32.lib")

using namespace Gdiplus;
using namespace std;

wstring current_path;

// Global Variables
HINSTANCE hInst;
HWND hSlider;
float zoomFactor = 1.0f;
const int defaultWidth = 960;
const int defaultHeight = 540;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

// Function Prototypes
ATOM RegisterAppClass(HINSTANCE hInstance);
BOOL InitializeApp(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Helper: Handle painting the window
void DrawImageInBoxWithZoom(HDC hdc, int boxX, int boxY, int boxWidth, int boxHeight, float zoom) {
    Graphics graphics(hdc);

    // Load the image
    Image image(current_path.c_str());

    // Get the original dimensions of the image
    UINT imgWidth = image.GetWidth();
    UINT imgHeight = image.GetHeight();

    // Calculate scaled dimensions
    int scaledWidth = static_cast<int>(imgWidth * zoom);
    int scaledHeight = static_cast<int>(imgHeight * zoom);

    // Calculate offsets to center the image within the box
    int offsetX = boxX - (scaledWidth - boxWidth) / 2;
    int offsetY = boxY - (scaledHeight - boxHeight) / 2;

    // Create a clipping region to mask the image
    Region clipRegion(Rect(boxX, boxY, boxWidth, boxHeight));
    graphics.SetClip(&clipRegion);

    // Draw the rectangle
    Rectangle(hdc, boxX, boxY, boxX + boxWidth, boxY + boxHeight);

    // Draw the image with scaling
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
        0, 0, defaultWidth, defaultHeight, nullptr, nullptr, hInstance, nullptr);

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
            OPENFILENAME ofn = { sizeof(OPENFILENAME) };
            wstring path(MAX_PATH, '\0');

            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = &path[0];
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = TEXT("All Images\0*.BMP;*.JPG;*.PNG;*.GIF;*.TIFF\0All Files\0*.*\0");
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn)) {
                path.resize(wcslen(path.c_str()));
                current_path = path;
                RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE);
                InvalidateRect(hWnd, NULL, TRUE);
            }
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
        return 0;
    }
    case WM_SIZE: {
        // Get window dimensions
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int windowWidth = clientRect.right - clientRect.left;
        int windowHeight = clientRect.bottom - clientRect.top;

        // Box dimensions (vertically centered, left-aligned)
        int boxWidth = static_cast<int>(windowWidth * 0.54);
        int boxHeight = static_cast<int>(windowHeight * 0.90);
        int boxX = 0; // Left-aligned
        int boxY = (windowHeight - boxHeight) / 2;

        // Update slider position and size
        int sliderWidth = 25; // Fixed slider width
        int sliderHeight = boxHeight;
        int sliderX = boxWidth; // Right of the box
        int sliderY = boxY;

        SetWindowPos(hSlider, NULL, sliderX, sliderY, sliderWidth, sliderHeight, SWP_NOZORDER);
        InvalidateRect(hWnd, NULL, TRUE); // Trigger redraw
        return 0;
    }
    case WM_HSCROLL: // Handle slider changes
    case WM_VSCROLL: {
        if ((HWND)lParam == hSlider) {
            // Get the slider position
            int sliderPos = SendMessage(hSlider, TBM_GETPOS, 0, 0);
            zoomFactor = 1.0f + (sliderPos - 50) / 50.0f; // Map slider to zoom range [0.5x, 2.0x]
            InvalidateRect(hWnd, NULL, TRUE); // Redraw the window
        }
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
    return DefWindowProc(hWnd, message, wParam, lParam);
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