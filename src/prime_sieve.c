#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * Simple implementation of the Sieve of Eratosthenes.
 * Usage: compile and run with a single integer argument N.
 * Prints all prime numbers up to and including N.
 */

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s N\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    if (N < 2) {
        fprintf(stderr, "N must be >= 2\n");
        return 1;
    }

    bool *is_prime = calloc(N + 1, sizeof(bool));
    if (!is_prime) {
        perror("calloc");
        return 1;
    }

    for (int i = 2; i <= N; ++i) {
        is_prime[i] = true;
    }

    for (int p = 2; p * p <= N; ++p) {
        if (is_prime[p]) {
            for (int multiple = p * p; multiple <= N; multiple += p) {
                is_prime[multiple] = false;
            }
        }
    }

    for (int i = 2; i <= N; ++i) {
        if (is_prime[i]) {
            printf("%d\n", i);
        }
    }

    free(is_prime);
    return 0;
}
