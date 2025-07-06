#include <stdio.h>
#include <stdlib.h>

// Generate a Euclidean rhythm pattern with the given number of steps and pulses.
// Prints a sequence of 1s and 0s representing beats and rests.
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <steps> <pulses>\n", argv[0]);
        return 1;
    }

    int steps = atoi(argv[1]);
    int pulses = atoi(argv[2]);

    if (steps <= 0) {
        fprintf(stderr, "Steps must be greater than 0.\n");
        return 1;
    }

    if (pulses < 0) {
        pulses = 0;
    } else if (pulses > steps) {
        pulses = steps;
    }

    for (int i = 0; i < steps; ++i) {
        int beat = ((pulses * i) % steps) < pulses ? 1 : 0;
        printf("%d", beat);
        if (i < steps - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}
