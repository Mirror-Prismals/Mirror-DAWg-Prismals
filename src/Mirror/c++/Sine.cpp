#include <windows.h>
#include <mmsystem.h>
#include <cmath>
#include <vector>
#pragma comment(lib, "winmm.lib")

const double PI = 3.14159265358979323846;
const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = SAMPLE_RATE * 2;  // 2 seconds

int main() {
    // Create buffer for our sound
    std::vector<short> buffer(BUFFER_SIZE);

    // Generate a 440Hz sine wave (A4 note)
    double frequency = 440.0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        double time = (double)i / SAMPLE_RATE;
        buffer[i] = (short)(32767.0 * sin(2.0 * PI * frequency * time));
    }

    // Prepare WAVEFORMATEX structure
    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1;  // Mono
    waveFormat.nSamplesPerSec = SAMPLE_RATE;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    // Open waveOut device
    HWAVEOUT hWaveOut;
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        MessageBox(NULL, L"Failed to open audio device", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Prepare WAVEHDR
    WAVEHDR waveHdr;
    waveHdr.lpData = (LPSTR)buffer.data();
    waveHdr.dwBufferLength = BUFFER_SIZE * sizeof(short);
    waveHdr.dwFlags = 0;
    waveHdr.dwLoops = 0;

    // Prepare header
    if (waveOutPrepareHeader(hWaveOut, &waveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        MessageBox(NULL, L"Failed to prepare header", L"Error", MB_OK | MB_ICONERROR);
        waveOutClose(hWaveOut);
        return -1;
    }

    // Play sound
    if (waveOutWrite(hWaveOut, &waveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        MessageBox(NULL, L"Failed to write audio data", L"Error", MB_OK | MB_ICONERROR);
        waveOutUnprepareHeader(hWaveOut, &waveHdr, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);
        return -1;
    }

    // Wait for playback to finish
    Sleep(2000);  // Wait 2 seconds

    // Clean up
    waveOutUnprepareHeader(hWaveOut, &waveHdr, sizeof(WAVEHDR));
    waveOutClose(hWaveOut);

    return 0;
}
