#include <windows.h>
#include <atlbase.h>
#include <sapi.h>
#include <iostream>
#include <sstream>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "sapi.lib")

struct Phrase {
    std::wstring word;
    int holdUnits;  // note length in 16th
    int restUnits;  // dots as rests *after*
};

int main() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    CComPtr<ISpVoice> voice;
    voice.CoCreateInstance(CLSID_SpVoice);
    voice->SetVolume(100);

    //-----------------------------------
    // Settings
    //-----------------------------------
    double bpm = 120;
    double ms_per_quarter = 60000.0 / bpm;
    double ms_per_sixteenth = ms_per_quarter / 4.0;

    std::wstring input =
        L"Okay - ------------------- - . . . now------------------------ what -- this --- is - full -- MIDA --- speaker . do . you -- understand ----------------------------------";

    // Step 1: split to blocks by whitespace
    std::wistringstream ss(input);
    std::vector<std::wstring> tokens;
    std::wstring tok;
    while (ss >> tok) tokens.push_back(tok);

    // Step 2: group tokens that belong together
    std::vector<Phrase> phrases;
    for (size_t i = 0; i < tokens.size(); ) {
        if (tokens[i] == L"-" || tokens[i] == L".") {
            ++i;  // skip any isolated -/. (unlikely)
            continue;
        }

        // it's a word or a word+suffixes
        std::wstring word_token = tokens[i++];
        int dashCount = 0, dotCount = 0;

        // gobble _trailing tokens_ attached to this word
        while (i < tokens.size()) {
            std::wstring t = tokens[i];
            if (t.find_first_not_of(L"-.") != std::wstring::npos) break;  // next word
            for (wchar_t ch : t) {
                if (ch == L'-') dashCount++;
                if (ch == L'.') dotCount++;
            }
            ++i;
        }

        // Count embedded dashes/dots inside this word_token if any (like now------------------------)
        int word_dash = 0, word_dot = 0;
        size_t wlen = word_token.length();
        size_t j = wlen;
        while (j > 0) {
            --j;
            if (word_token[j] == L'-') { word_dash++; continue; }
            if (word_token[j] == L'.') { word_dot++; continue; }
            else break;
        }

        int totalDashes = word_dash + dashCount;
        int totalDots = word_dot + dotCount;

        std::wstring word = word_token.substr(0, wlen - word_dash - word_dot);
        if (word.empty()) continue; // ignore lone dashes or dots

        Phrase p{ word, 1 + totalDashes, totalDots };
        phrases.push_back(p);
    }

    // Playback
    for (auto& p : phrases) {
        int total_note_ms = int(p.holdUnits * ms_per_sixteenth);
        int total_rest_ms = int(p.restUnits * ms_per_sixteenth);

        // Pick rate scale for holding the word longer/slower
        int rate = 0;
        if (p.holdUnits >= 12) rate = -10;
        else if (p.holdUnits >= 8) rate = -8;
        else if (p.holdUnits >= 6) rate = -6;
        else if (p.holdUnits >= 4) rate = -4;
        else if (p.holdUnits >= 2) rate = -2;
        else rate = 0;

        voice->SetRate(rate);

        ULONGLONG start = GetTickCount64();
        voice->Speak(p.word.c_str(), SPF_ASYNC, nullptr);
        Sleep(total_note_ms);
        voice->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);  // cut off

        if (total_rest_ms > 0) Sleep(total_rest_ms);
    }

    voice.Release();
    CoUninitialize();
    return 0;
}
