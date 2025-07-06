#include <stdio.h>
#include <limits.h>
#include <stdbool.h>

#define V 6

static int min_distance(int dist[V], bool visited[V]) {
    int min = INT_MAX;
    int min_idx = -1;
    for (int v = 0; v < V; ++v) {
        if (!visited[v] && dist[v] <= min) {
            min = dist[v];
            min_idx = v;
        }
    }
    return min_idx;
}

static void dijkstra(int graph[V][V], int src, int dist[V]) {
    bool visited[V] = {false};
    for (int i = 0; i < V; ++i)
        dist[i] = INT_MAX;
    dist[src] = 0;

    for (int count = 0; count < V - 1; ++count) {
        int u = min_distance(dist, visited);
        if (u == -1)
            break;
        visited[u] = true;
        for (int v = 0; v < V; ++v) {
            if (!visited[v] && graph[u][v] &&
                dist[u] != INT_MAX &&
                dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
            }
        }
    }
}

int main(void) {
    int graph[V][V] = {
        {0, 7, 9, 0, 0,14},
        {7, 0,10,15, 0, 0},
        {9,10, 0,11, 0, 2},
        {0,15,11, 0, 6, 0},
        {0, 0, 0, 6, 0, 9},
        {14,0, 2, 0, 9, 0}
    };

    int dist[V];
    dijkstra(graph, 0, dist);

    printf("Shortest distances from node 0:\n");
    for (int i = 0; i < V; ++i)
        printf("0 -> %d = %d\n", i, dist[i]);

    return 0;
}
