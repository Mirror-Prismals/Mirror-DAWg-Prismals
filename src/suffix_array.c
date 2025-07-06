#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char *g_s;   // string
static int g_n;                     // length
static int g_k;                     // current step
static int *g_rank;                 // ranks for comparison

// Comparator for qsort using the doubling technique
static int cmp_sa(const void *a, const void *b)
{
    int i = *(const int *)a;
    int j = *(const int *)b;
    if (g_rank[i] != g_rank[j])
        return g_rank[i] - g_rank[j];
    int ri = (i + g_k < g_n) ? g_rank[i + g_k] : -1;
    int rj = (j + g_k < g_n) ? g_rank[j + g_k] : -1;
    return ri - rj;
}

// Build suffix array using the doubling algorithm
void build_suffix_array(const unsigned char *s, int n, int *sa)
{
    g_s = s;
    g_n = n;
    g_rank = malloc(n * sizeof(int));
    int *tmp = malloc(n * sizeof(int));
    if (!g_rank || !tmp)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; ++i)
    {
        sa[i] = i;
        g_rank[i] = s[i];
    }

    for (g_k = 1;; g_k <<= 1)
    {
        qsort(sa, n, sizeof(int), cmp_sa);

        tmp[sa[0]] = 0;
        for (int i = 1; i < n; ++i)
            tmp[sa[i]] = tmp[sa[i - 1]] + (cmp_sa(&sa[i - 1], &sa[i]) < 0);

        for (int i = 0; i < n; ++i)
            g_rank[i] = tmp[i];

        if (g_rank[sa[n - 1]] == n - 1)
            break;
    }

    free(tmp);
    free(g_rank);
}

// Build LCP array using Kasai's algorithm
void build_lcp_array(const unsigned char *s, int n, const int *sa, int *lcp)
{
    int *rank = malloc(n * sizeof(int));
    if (!rank)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; ++i)
        rank[sa[i]] = i;

    int h = 0;
    for (int i = 0; i < n; ++i)
    {
        int r = rank[i];
        if (r > 0)
        {
            int j = sa[r - 1];
            while (i + h < n && j + h < n && s[i + h] == s[j + h])
                ++h;
            lcp[r] = h;
            if (h > 0)
                --h;
        }
        else
        {
            lcp[r] = 0;
        }
    }

    free(rank);
}

int main(int argc, char **argv)
{
    char buffer[100000];
    const unsigned char *s;

    if (argc > 1)
    {
        s = (const unsigned char *)argv[1];
    }
    else
    {
        if (!fgets(buffer, sizeof(buffer), stdin))
            return 0;
        buffer[strcspn(buffer, "\n")] = '\0';
        s = (const unsigned char *)buffer;
    }

    int n = strlen((const char *)s);
    if (n == 0)
        return 0;

    int *sa = malloc(n * sizeof(int));
    int *lcp = malloc(n * sizeof(int));
    if (!sa || !lcp)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    build_suffix_array(s, n, sa);
    build_lcp_array(s, n, sa, lcp);

    printf("Suffix Array:\n");
    for (int i = 0; i < n; ++i)
        printf("%d%c", sa[i], i + 1 == n ? '\n' : ' ');

    printf("LCP Array:\n");
    for (int i = 1; i < n; ++i)
        printf("%d%c", lcp[i], i + 1 == n ? '\n' : ' ');

    free(sa);
    free(lcp);
    return 0;
}

