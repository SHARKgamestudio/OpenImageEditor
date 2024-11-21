#include "BitmapSaver.h"

#include <codecvt>
#include <iostream>

int GetEncoderClsid(const WCHAR* format, CLSID& clsid) {
    UINT num = 0;
    UINT size = 0;

    ImageCodecInfo* imageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;

    imageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (imageCodecInfo == NULL)
        return -1;

    GetImageEncoders(num, size, imageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(imageCodecInfo[j].MimeType, format) == 0) {
            clsid = imageCodecInfo[j].Clsid;
            free(imageCodecInfo);
            return j;
        }
    }

    free(imageCodecInfo);
    return -1;
}

bool SaveImage(Bitmap* image, const wstring& path) {
    if (path.empty()) {
        wcerr << L"Output path is empty.\n";
        return false;
    }

    CLSID encoderClsid;
    if (GetEncoderClsid(L"image/png", encoderClsid) == -1 && GetEncoderClsid(L"image/jpeg", encoderClsid) == -1) {
        wcerr << L"Failed to get encoder CLSID.\n";
        return false;
    }

    if (image->Save(path.c_str(), &encoderClsid, nullptr) != Ok) {
        wcerr << L"Failed to save the image at " << path << L"\n";
        return false;
    }

    return true;
}