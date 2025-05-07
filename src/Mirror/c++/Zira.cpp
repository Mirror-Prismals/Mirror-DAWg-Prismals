// =======================
//  SpeakDemo.cpp
// =======================

// Disable annoying deprecated API warnings INSIDE Windows headers
#define _CRT_SECURE_NO_WARNINGS
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#pragma warning(disable: 4996)  // Suppress deprecated warning

#include <windows.h>
#include <atlbase.h>
#include <sapi.h>
#include <sphelper.h>    // For SpEnumTokens, SpGetDescription

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "sapi.lib")

#include <iostream>

int main() {
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "CoInitialize failed" << std::endl;
        return -1;
    }

    CComPtr<ISpVoice> voice;
    hr = voice.CoCreateInstance(CLSID_SpVoice);
    if (FAILED(hr)) {
        std::cerr << "Failed to create voice" << std::endl;
        return -1;
    }

    // Find Microsoft Zira
    CComPtr<IEnumSpObjectTokens> cpEnum;
    ULONG count;
    hr = SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &cpEnum);
    if (SUCCEEDED(hr)) {
        cpEnum->GetCount(&count);
        CComPtr<ISpObjectToken> chosenToken;
        for (ULONG i = 0; i < count; ++i) {
            CComPtr<ISpObjectToken> token;
            if (SUCCEEDED(cpEnum->Next(1, &token, nullptr))) {
                WCHAR* desc = nullptr;
                if (SUCCEEDED(SpGetDescription(token, &desc))) {
                    // Print to console
                    wprintf(L"Voice %u: %s\n", i, desc);
                    if (wcsstr(desc, L"Zira")) {  // pick Zira voice
                        chosenToken = token;
                    }
                    ::CoTaskMemFree(desc);
                }
            }
        }
        if (chosenToken) {
            voice->SetVoice(chosenToken);
            std::wcout << L"Using Microsoft Zira voice.\n";
        }
        else {
            std::wcout << L"Zira not found. Using default voice.\n";
        }
    }
    else {
        std::cerr << "Failed to enumerate voices." << std::endl;
    }

    // Set volume and rate (optional)
    voice->SetRate(0);     // -10 (slow) to 10 (fast). 0=normal
    voice->SetVolume(100); // 0..100%

    // Speak async or sync
    std::wcout << L"\nSpeaking phrase:\n";
    voice->Speak(L"Hello Zack! This is Zira speaking with MIDA timing.", SPF_DEFAULT, nullptr);

    voice.Release();
    CoUninitialize();
    return 0;
}
