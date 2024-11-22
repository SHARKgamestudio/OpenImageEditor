#include "FileExplorer.h"

#pragma region External Dependencies

#include <stdexcept>

#pragma endregion

using namespace std;

// Constructor to initialize the FileExplorer with owner handle and filters
FileExplorer::FileExplorer(HWND owner, const wchar_t* filters) {
    if (!owner)   { throw invalid_argument("Owner window handle cannot be null."); }
    if (!filters) { throw invalid_argument("Filters cannot be null."); }

    // Initialize OPENFILENAME structure
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = &path[0];
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filters;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

// Opens a file dialog and returns true if a file is selected, false otherwise
bool FileExplorer::OpenFile() {
    if (GetOpenFileName(&ofn)) {
        try {
            // Resize the path string to the correct length
            path.resize(wcslen(path.c_str()));
            return true;
        }
        catch (const std::exception& e) {
            // Log the exception and return false
            // Replace with proper logging if required
            OutputDebugStringA(e.what());
            return false;
        }
    }
    else {
        // Handle error or cancellation
        DWORD error = CommDlgExtendedError();
        if (error != 0) {
            // Log or handle specific errors if needed
            OutputDebugStringA("Error opening file dialog.");
        }
        return false;
    }
}

// Opens a save file dialog and appends .png if not present
bool FileExplorer::SaveFile() {
    if (GetSaveFileName(&ofn)) {
        try {
            // Resize the path string to the correct length
            path.resize(wcslen(path.c_str()));

            // Ensure the file has a .png extension
            if (path.find(L".png") == std::wstring::npos) {
                path += L".png";
            }
            return true;
        }
        catch (const std::exception& e) {
            // Log the exception and return false
            OutputDebugStringA(e.what());
            return false;
        }
    }
    else {
        // Handle error or cancellation
        DWORD error = CommDlgExtendedError();
        if (error != 0) {
            // Log or handle specific errors if needed
            OutputDebugStringA("Error saving file dialog.");
        }
        return false;
    }
}

// Retrieves the file path
wstring FileExplorer::GetPath() {
    return path;
}