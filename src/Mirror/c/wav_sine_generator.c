#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
 * wav_sine_generator.c
 * ---------------------
 * Generate a mono 16-bit PCM WAV file containing a sine wave.
 *
 * Usage:
 *   wav_sine_generator output.wav frequency duration [sample_rate]
 *
 *   output.wav   - path to output file
 *   frequency    - frequency of sine wave in Hz
 *   duration     - duration of audio in seconds
 *   sample_rate  - optional sample rate (default: 44100)
 */

static void write_little_endian_uint32(FILE *f, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        fputc((value >> (8 * i)) & 0xFF, f);
    }
}

static void write_little_endian_uint16(FILE *f, uint16_t value) {
    for (int i = 0; i < 2; ++i) {
        fputc((value >> (8 * i)) & 0xFF, f);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s output.wav frequency duration [sample_rate]\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    double frequency = atof(argv[2]);
    double duration = atof(argv[3]);
    uint32_t sample_rate = (argc > 4) ? (uint32_t)atoi(argv[4]) : 44100;

    if (frequency <= 0 || duration <= 0 || sample_rate == 0) {
        fprintf(stderr, "Invalid parameters.\n");
        return 1;
    }

    uint16_t channels = 1;
    uint16_t bits_per_sample = 16;
    uint32_t num_samples = (uint32_t)(duration * sample_rate);
    uint32_t byte_rate = sample_rate * channels * bits_per_sample / 8;
    uint16_t block_align = channels * bits_per_sample / 8;
    uint32_t data_size = num_samples * block_align;

    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    /* Write WAV header */
    fputs("RIFF", f);
    write_little_endian_uint32(f, 36 + data_size); /* Chunk size */
    fputs("WAVE", f);

    /* fmt chunk */
    fputs("fmt ", f);
    write_little_endian_uint32(f, 16); /* Subchunk1Size */
    write_little_endian_uint16(f, 1);  /* PCM format */
    write_little_endian_uint16(f, channels);
    write_little_endian_uint32(f, sample_rate);
    write_little_endian_uint32(f, byte_rate);
    write_little_endian_uint16(f, block_align);
    write_little_endian_uint16(f, bits_per_sample);

    /* data chunk */
    fputs("data", f);
    write_little_endian_uint32(f, data_size);

    /* Generate samples */
    double amplitude = 0.8; /* 80% of full scale */
    const double two_pi = 2.0 * M_PI;
    for (uint32_t i = 0; i < num_samples; ++i) {
        double t = (double)i / sample_rate;
        double sample = amplitude * sin(two_pi * frequency * t);
        int16_t s = (int16_t)(sample * 32767);
        write_little_endian_uint16(f, (uint16_t)s);
    }

    fclose(f);
    printf("WAV file '%s' generated.\n", filename);
    return 0;
}
