#include "LSB.h"

#pragma region External Dependencies

#include <vector>
#include <codecvt>
#include <iostream>
#include <stdexcept>

#pragma endregion

using namespace std;

// Utility function to convert std::string to std::wstring
wstring StringToWString(const string& str) {
    try {
        wstring_convert<codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(str);
    }
    catch (const exception& e) {
        wcerr << L"Error converting string to wstring: " << e.what() << endl;
        return L"";
    }
}

// Utility function to convert wchar_t* to std::string
string WCharToString(const wchar_t* wcharBuffer) {
    try {
        wstring_convert<codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wcharBuffer);
    }
    catch (const exception& e) {
        cerr << "Error converting wchar_t* to string: " << e.what() << endl;
        return "";
    }
}

// Function to encode a message into an image using LSB steganography
bool EncodeMessage(const wstring& path, const wchar_t* message, Bitmap*& output) {
    // Convert the message to a std::string
    string str_message = WCharToString(message);
    if (str_message.empty()) {
        wcerr << L"Message conversion failed.\n";
        return false;
    }

    // Load the image
    Bitmap* image = new Bitmap(path.c_str());
    if (image->GetLastStatus() != Ok) {
        wcerr << L"Failed to load image.\n";
        delete image;
        return false;
    }

    // Prepare the data to embed
    vector<uint8_t> data(str_message.begin(), str_message.end());
    data.push_back('\0'); // Null-terminate the message

    UINT width = image->GetWidth(), height = image->GetHeight();
    size_t dataIdx = 0, bitCount = 0;

    // Embed the message bits into the image
    try {
        for (UINT y = 0; y < height; ++y) {
            for (UINT x = 0; x < width; ++x) {
                if (dataIdx < data.size()) {
                    Color pixel;
                    if (image->GetPixel(x, y, &pixel) != Ok) {
                        throw runtime_error("Failed to get pixel.");
                    }

                    // Modify the least significant bit of the blue channel
                    uint8_t bit = (data[dataIdx] >> (7 - bitCount)) & 1;
                    BYTE newBlue = (pixel.GetBlue() & 0xFE) | bit;
                    Color newPixel(pixel.GetA(), pixel.GetR(), pixel.GetG(), newBlue);

                    if (image->SetPixel(x, y, newPixel) != Ok) {
                        throw runtime_error("Failed to set pixel.");
                    }

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
    }
    catch (const exception& e) {
        wcerr << L"Error during encoding: " << e.what() << endl;
        delete image;
        return false;
    }

    output = image;
    return true;
}

// Function to decode a message from an image
wstring DecodeMessage(const wstring& path) {
    // Load the image
    Bitmap image(path.c_str());
    if (image.GetLastStatus() != Ok) {
        wcerr << L"Failed to load image.\n";
        return L"";
    }

    UINT width = image.GetWidth(), height = image.GetHeight();
    vector<uint8_t> messageData;
    uint8_t currentByte = 0;
    size_t bitCount = 0;

    // Extract the embedded bits
    try {
        for (UINT y = 0; y < height; ++y) {
            for (UINT x = 0; x < width; ++x) {
                Color pixel;
                if (image.GetPixel(x, y, &pixel) != Ok) {
                    throw runtime_error("Failed to get pixel.");
                }

                uint8_t lsb = pixel.GetBlue() & 0x01;
                currentByte |= (lsb << (7 - bitCount));
                bitCount++;

                if (bitCount == 8) {
                    messageData.push_back(currentByte);
                    if (currentByte == '\0') break; // Stop at null terminator
                    currentByte = 0;
                    bitCount = 0;
                }
            }
        }
    }
    catch (const exception& e) {
        wcerr << L"Error during decoding: " << e.what() << endl;
        return L"";
    }

    // Convert the extracted message to a string
    string decodedMessage(messageData.begin(), messageData.end());
    size_t nullPos = decodedMessage.find('\0');
    if (nullPos != string::npos) {
        decodedMessage = decodedMessage.substr(0, nullPos);
    }

    return StringToWString(decodedMessage);
}