#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// Simple segmented sieve implementation that generates all prime numbers up to N.
// Usage: ./segmented_sieve <N>
// Prints primes separated by spaces.

static void simple_sieve(int limit, bool *is_prime, int *primes, int *count) {
    for (int i = 0; i <= limit; ++i) is_prime[i] = true;
    is_prime[0] = is_prime[1] = false;
    for (int p = 2; p * p <= limit; ++p) {
        if (is_prime[p]) {
            for (int i = p * p; i <= limit; i += p) is_prime[i] = false;
        }
    }
    *count = 0;
    for (int i = 2; i <= limit; ++i) if (is_prime[i]) primes[(*count)++] = i;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <limit>\n", argv[0]);
        return 1;
    }
    long long n = atoll(argv[1]);
    if (n < 2) return 0;

    int limit = (int)floor(sqrt((double)n)) + 1;
    bool *mark = malloc((limit + 1) * sizeof(bool));
    int *primes = malloc((limit + 1) * sizeof(int));
    int count = 0;
    simple_sieve(limit, mark, primes, &count);
    free(mark);

    // Print primes found by the simple sieve first
    for (int i = 0; i < count; ++i) printf("%d ", primes[i]);

    long long low = limit;
    long long high = low + limit;
    mark = malloc((limit + 1) * sizeof(bool));
    while (low <= n) {
        if (high > n + 1) high = n + 1;
        for (int i = 0; i < high - low; ++i) mark[i] = true;

        for (int i = 0; i < count; ++i) {
            int p = primes[i];
            long long start = (low + p - 1) / p * p;
            for (long long j = start; j < high; j += p) mark[j - low] = false;
        }

        for (long long i = low; i < high; ++i)
            if (mark[i - low])
                printf("%lld ", i);

        low += limit;
        high += limit;
    }

    printf("\n");

    free(primes);
    free(mark);
    return 0;
}
