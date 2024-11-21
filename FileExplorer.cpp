#include "FileExplorer.h"

FileExplorer::FileExplorer(HWND owner, const wchar_t* filters) {
    ofn.hwndOwner = owner;
    ofn.lpstrFile = &path[0];
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filters;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}

bool FileExplorer::OpenFile() {
    if (GetOpenFileName(&ofn)) {
        path.resize(wcslen(path.c_str()));
		return true;
    }
    else { return false; }
}

bool FileExplorer::SaveFile() {
    if (GetSaveFileName(&ofn)) {
        path.resize(wcslen(path.c_str()));
        if (!path.find(L".png")) {
			path += L".png";
        }
        return true;
    }
    else { return false; }
}

wstring FileExplorer::GetPath() {
    return path;
}