#include "LSB.h"

#include <vector>
#include <codecvt>
#include <iostream>

wstring StringToWString(const std::string& str) {
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

string WCharToString(const wchar_t* wcharBuffer) {
    wstring_convert<codecvt_utf8<wchar_t>> converter;
    string utf8String = converter.to_bytes(wcharBuffer);
    return utf8String;
}

bool EncodeMessage(const wstring& path, const wchar_t* message, Bitmap*& output) {

	string str_message = WCharToString(message);

    Bitmap* image = new Bitmap(path.c_str());
    if (image->GetLastStatus() != Ok) {
        wcerr << L"Failed to load image.\n";
        delete image;
        return false;
    }

    vector<uint8_t> data(str_message.begin(), str_message.end());
    data.push_back('\0');

    UINT width = image->GetWidth(), height = image->GetHeight();
    size_t dataIdx = 0, bitCount = 0;

    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            if (dataIdx < data.size()) {
                Color pixel;
                image->GetPixel(x, y, &pixel);
                uint8_t bit = (data[dataIdx] >> (7 - bitCount)) & 1;
                BYTE newBlue = (pixel.GetBlue() & 0xFE) | bit;
                Color newPixel(pixel.GetA(), pixel.GetR(), pixel.GetG(), newBlue);
                image->SetPixel(x, y, newPixel);

                bitCount++;
                if (bitCount == 8) {
                    bitCount = 0;
                    dataIdx++;
                }
            }
            else {
                break;
            }
        }
    }

    output = image;
    return true;
}

wstring DecodeMessage(const wstring& path) {
    Bitmap image(path.c_str());
    if (image.GetLastStatus() != Ok) {
        wcerr << L"Failed to load image.\n";
        return L"";
    }

    UINT width = image.GetWidth(), height = image.GetHeight();
    vector<uint8_t> messageData;
    uint8_t currentByte = 0;
    size_t bitCount = 0;

    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            Color pixel;
            image.GetPixel(x, y, &pixel);
            uint8_t lsb = pixel.GetBlue() & 0x01;
            currentByte |= (lsb << (7 - bitCount));
            bitCount++;
            if (bitCount == 8) {
                messageData.push_back(currentByte);
                currentByte = 0;
                bitCount = 0;
            }
        }
    }

    string decodedMessage(messageData.begin(), messageData.end());
    size_t nullPos = decodedMessage.find('\0');
    if (nullPos != std::string::npos) {
        decodedMessage = decodedMessage.substr(0, nullPos);
    }

    return StringToWString(decodedMessage);
}