#pragma once

#include <Windows.h>
#include <gdiplus.h>
#include <string>

using namespace Gdiplus;
using namespace std;

bool SaveImage(Bitmap* image, const wstring& path);