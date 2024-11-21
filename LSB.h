#pragma once

#include <Windows.h>
#include <gdiplus.h>
#include <string>

using namespace Gdiplus;
using namespace std;

bool EncodeMessage(const wstring& path, const wchar_t* message, Bitmap*& output);
wstring DecodeMessage(const wstring& path);