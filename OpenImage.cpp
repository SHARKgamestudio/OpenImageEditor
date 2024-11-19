// OpenImage.cpp : Defines the entry point for the application.

#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <string>
#include <codecvt>

#include <vector>
#include <iostream>

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

HWND hEdit;

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

bool EncodeMessageInImage(const std::wstring& imagePath, const std::string& message, Bitmap*& outputImage) {
    // Load the image
    Bitmap* image = new Bitmap(imagePath.c_str());
    if (image->GetLastStatus() != Ok) {
        std::wcerr << L"Failed to load image.\n";
        delete image;
        return false;
    }

    // Prepare the message to encode
    std::vector<uint8_t> data(message.begin(), message.end());
    data.push_back('\0'); // Null-terminate the message

    UINT width = image->GetWidth();
    UINT height = image->GetHeight();
    size_t dataIdx = 0;
    size_t bitCount = 0;

    // Encode message into the blue channel
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            if (dataIdx < data.size()) {
                Color pixel;
                image->GetPixel(x, y, &pixel);

                // Get the current bit from the message
                uint8_t bit = (data[dataIdx] >> (7 - bitCount)) & 1;

                // Modify the least significant bit of the blue channel
                BYTE newBlue = (pixel.GetBlue() & 0xFE) | bit;
                Color newPixel(pixel.GetA(), pixel.GetR(), pixel.GetG(), newBlue);
                image->SetPixel(x, y, newPixel);

                // Move to the next bit of the message
                bitCount++;
                if (bitCount == 8) {
                    bitCount = 0;
                    dataIdx++;
                }
            }
            else {
                // If all message data is encoded, break the loop
                break;
            }
        }
    }

    outputImage = image;
    return true;
}

int GetEncoderClsid(const WCHAR* format, CLSID& clsid) {
    UINT num = 0;
    UINT size = 0;

    // Get the number of image encoders
    ImageCodecInfo* imageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;

    imageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (imageCodecInfo == NULL)
        return -1;

    // Get the available encoders
    GetImageEncoders(num, size, imageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        // Compare the format string to the MimeType
        if (wcscmp(imageCodecInfo[j].MimeType, format) == 0) {
            clsid = imageCodecInfo[j].Clsid;
            free(imageCodecInfo);
            return j;  // Success
        }
    }

    free(imageCodecInfo);
    return -1;  // Failure
}

// Function to save the image
bool SaveImage(Bitmap* image, const std::wstring& outputPath) {
    // Check if the path is valid
    if (outputPath.empty()) {
        //std::wcerr << L"Output path is empty.\n";
        return false;
    }

    // Attempt to save the image in the best format (PNG or JPEG)
    CLSID encoderClsid;
    if (GetEncoderClsid(L"image/png", encoderClsid) == -1 && GetEncoderClsid(L"image/jpeg", encoderClsid) == -1) {
        //std::wcerr << L"Failed to get encoder CLSID.\n";
        return false;
    }

    // Attempt to save the image to disk
    if (image->Save(outputPath.c_str(), &encoderClsid, nullptr) != Ok) {
        //std::wcerr << L"Failed to save the image at " << outputPath << L"\n";
        return false;
    }

    return true;
}

wstring stringToWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

string wcharToString(const wchar_t* wcharBuffer) {
    // Use a wstring_convert to convert the wchar_t buffer to a UTF-8 string
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string utf8String = converter.to_bytes(wcharBuffer);
    return utf8String;
}

wstring DecodeMessageFromImage(const std::wstring& imagePath) {
    // Load the image
    Bitmap image(imagePath.c_str());
    if (image.GetLastStatus() != Ok) {
        std::wcerr << L"Failed to load image.\n";
        return L"";
    }

    // Prepare to read the LSBs
    UINT width = image.GetWidth();
    UINT height = image.GetHeight();

    std::vector<uint8_t> messageData;
    uint8_t currentByte = 0;
    size_t bitCount = 0;

    // Read each pixel and extract the LSB from the blue channel
    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            Color pixel;
            image.GetPixel(x, y, &pixel);

            // Extract the least significant bit of the blue channel
            uint8_t lsb = pixel.GetBlue() & 0x01;

            // Add the bit to the current byte
            currentByte |= (lsb << (7 - bitCount));
            bitCount++;

            // Once we have 8 bits (1 byte), store it in the messageData vector
            if (bitCount == 8) {
                messageData.push_back(currentByte);
                currentByte = 0;
                bitCount = 0;
            }
        }
    }

    // Convert the message data into a string
    std::string decodedMessage(messageData.begin(), messageData.end());

    // Find and remove the null-terminator (if any)
    size_t nullPos = decodedMessage.find('\0');
    if (nullPos != std::string::npos) {
        decodedMessage = decodedMessage.substr(0, nullPos);
    }

    return stringToWString(decodedMessage);
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
        case ID_BUTTON: {
            wchar_t buffer[256]; // Allocate a buffer to store the text
            GetWindowText(hEdit, buffer, sizeof(buffer)); // Get the text from the Edit control

            Bitmap* encodedImage = nullptr;
            if (EncodeMessageInImage(current_path, wcharToString(buffer), encodedImage)) {
                if (SaveImage(encodedImage, L"C:/Users/AMD/Downloads/encoded.png")) {
                    MessageBox(hWnd, L"Saved succesfuly !", L"Notif Encrypt", MB_OK);
                }
                delete encodedImage;
            }

            return 0;
        }
        case ID_EDIT: {
            // SOMETHING WHILE I ENTER STUFF IN THE EDIT BOX
            return 0;
        }
        case ID_WBUTTON2: {
            wstring message = DecodeMessageFromImage(current_path).c_str();
            MessageBox(hWnd, message.c_str(), L"Notif Decrypt", MB_OK);
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

        CreateWindow(
            L"BUTTON",              // Predefined class for buttons
            L"Encrypt",            // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
            550, 50, 100, 30,       // x, y, width, height
            hWnd,                  // Parent window handle
            (HMENU)ID_BUTTON,      // Button identifier
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL                   // Pointer to additional data
        );

        CreateWindow(
            L"BUTTON",              // Predefined class for buttons
            L"Decrypt",            // Button text
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
            550, 100, 100, 30,       // x, y, width, height
            hWnd,                  // Parent window handle
            (HMENU)ID_WBUTTON2,      // Button identifier
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL                   // Pointer to additional data
        );

        hEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,  // Style: adding a border effect
            L"EDIT",            // Class name for the edit control
            L"type something here",                // Initial text in the edit control
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, // Styles (visible, child, etc.)
            10, 10,            // Position (x, y)
            200, 25,           // Size (width, height)
            hWnd,              // Parent window
            (HMENU)ID_EDIT,          // Control ID
            (HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE),
            NULL
        );             // Additional creation data


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