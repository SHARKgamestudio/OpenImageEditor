#pragma once

#include <Windows.h>
#include <string>

using namespace std;

class FileExplorer {
private:
	OPENFILENAME ofn = { sizeof(OPENFILENAME) };
	wstring path = wstring(MAX_PATH, '\0');
public:
	FileExplorer(HWND owner, const wchar_t* filters);

	bool OpenFile();
	bool SaveFile();

	wstring GetPath();
};