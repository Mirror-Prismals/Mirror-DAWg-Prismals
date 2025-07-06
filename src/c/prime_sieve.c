#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * Simple implementation of the Sieve of Eratosthenes.
 * Usage: ./prime_sieve <limit>
 * Prints all prime numbers less than or equal to the given limit.
 */

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <limit>\n", argv[0]);
        return 1;
    }

    int limit = atoi(argv[1]);
    if (limit < 2) {
        printf("No primes <= %d\n", limit);
        return 0;
    }

    bool *is_prime = malloc((limit + 1) * sizeof(bool));
    if (!is_prime) {
        perror("malloc");
        return 1;
    }

    for (int i = 0; i <= limit; ++i) {
        is_prime[i] = true;
    }
    is_prime[0] = is_prime[1] = false;

    for (int p = 2; p * p <= limit; ++p) {
        if (is_prime[p]) {
            for (int i = p * p; i <= limit; i += p) {
                is_prime[i] = false;
            }
        }
    }

    for (int i = 2; i <= limit; ++i) {
        if (is_prime[i]) {
            printf("%d\n", i);
        }
    }

    free(is_prime);
    return 0;
}
