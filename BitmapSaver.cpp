#include "BitmapSaver.h"

#pragma region External Dependencies

#include <memory>
#include <codecvt>
#include <iostream>

#pragma endregion

using namespace std;

int GetEncoderClsid(const WCHAR* format, CLSID& clsid) {
    UINT num = 0;  // Number of image encoders
    UINT size = 0; // Size of the image encoder array

    // Get the size and number of image encoders
    GetImageEncodersSize(&num, &size);
    if (size == 0) {
        wcerr << L"Failed to retrieve image encoders size.\n";
        return -1;
    }

    // Allocate memory for image encoders
    unique_ptr<ImageCodecInfo[]> imageCodecInfo(reinterpret_cast<ImageCodecInfo*>(malloc(size)));
    if (!imageCodecInfo) {
        wcerr << L"Memory allocation failed for image encoders.\n";
        return -1;
    }

    // Get the image encoders
    GetImageEncoders(num, size, imageCodecInfo.get());

    // Search for the encoder matching the desired format
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(imageCodecInfo[j].MimeType, format) == 0) {
            clsid = imageCodecInfo[j].Clsid;
            return j; // Success
        }
    }

    wcerr << L"No matching encoder found for format: " << format << L"\n";
    return -1; // Encoder not found
}

bool SaveImage(Bitmap* image, const wstring& path) {
    if (!image) {
        wcerr << L"Image is null.\n";
        return false;
    }

    if (path.empty()) {
        wcerr << L"Output path is empty.\n";
        return false;
    }

    CLSID encoderClsid;

    // Attempt to get PNG encoder first, then fallback to JPEG
    if (GetEncoderClsid(L"image/png", encoderClsid) == -1) {
        if (GetEncoderClsid(L"image/jpeg", encoderClsid) == -1) {
            wcerr << L"Failed to get a valid encoder CLSID.\n";
            return false;
        }
    }

    // Attempt to save the image
    Status status = image->Save(path.c_str(), &encoderClsid, nullptr);
    if (status != Ok) {
        wcerr << L"Failed to save the image to " << path << L". Error code: " << status << L"\n";
        return false;
    }

    return true;
}