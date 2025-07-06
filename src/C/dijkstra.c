#include <stdio.h>
#include <limits.h>

#define V 5

/* Example graph represented as an adjacency matrix.
   A value of 0 means no edge between vertices. */
static int graph[V][V] = {
    {0, 10, 0, 5, 0},
    {0, 0, 1, 2, 0},
    {0, 0, 0, 0, 4},
    {0, 3, 9, 0, 2},
    {7, 0, 6, 0, 0}
};

/* Dijkstra's algorithm to compute the shortest path from src to
   all other vertices in the graph. */
static void dijkstra(int src)
{
    int dist[V];     /* shortest known distance from src to i */
    int visited[V];  /* vertex i has been processed */

    for (int i = 0; i < V; ++i) {
        dist[i] = INT_MAX;
        visited[i] = 0;
    }
    dist[src] = 0;

    for (int count = 0; count < V - 1; ++count) {
        /* pick the unvisited vertex with the smallest distance */
        int min = INT_MAX;
        int u = -1;
        for (int i = 0; i < V; ++i) {
            if (!visited[i] && dist[i] <= min) {
                min = dist[i];
                u = i;
            }
        }

        if (u == -1) /* remaining vertices are unreachable */
            break;

        visited[u] = 1;

        /* update distance value of the adjacent vertices */
        for (int v = 0; v < V; ++v) {
            if (graph[u][v] && !visited[v] && dist[u] != INT_MAX &&
                dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
            }
        }
    }

    printf("Vertex\tDistance from Source %d\n", src);
    for (int i = 0; i < V; ++i) {
        if (dist[i] == INT_MAX)
            printf("%d\tINF\n", i);
        else
            printf("%d\t%d\n", i, dist[i]);
    }
}

int main(void)
{
    int src = 0; /* change this to compute distances from another vertex */
    dijkstra(src);
    return 0;
}
