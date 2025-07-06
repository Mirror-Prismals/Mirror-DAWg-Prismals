#include <stdio.h>
#include <string.h>
#include <limits.h>

#define MAX_NODES 50

static int capacity[MAX_NODES][MAX_NODES];
static int residual[MAX_NODES][MAX_NODES];
static int parent[MAX_NODES];
static int visited[MAX_NODES];
static int node_count;

static int bfs(int s, int t) {
    memset(visited, 0, sizeof(visited));
    memset(parent, -1, sizeof(parent));

    int queue[MAX_NODES];
    int front = 0, back = 0;
    queue[back++] = s;
    visited[s] = 1;

    while (front < back) {
        int u = queue[front++];
        for (int v = 0; v < node_count; v++) {
            if (!visited[v] && residual[u][v] > 0) {
                queue[back++] = v;
                visited[v] = 1;
                parent[v] = u;
                if (v == t)
                    return 1;
            }
        }
    }
    return 0;
}

static int edmonds_karp(int s, int t) {
    for (int i = 0; i < node_count; i++)
        for (int j = 0; j < node_count; j++)
            residual[i][j] = capacity[i][j];

    int max_flow = 0;

    while (bfs(s, t)) {
        int path_flow = INT_MAX;
        for (int v = t; v != s; v = parent[v]) {
            int u = parent[v];
            if (residual[u][v] < path_flow)
                path_flow = residual[u][v];
        }
        for (int v = t; v != s; v = parent[v]) {
            int u = parent[v];
            residual[u][v] -= path_flow;
            residual[v][u] += path_flow;
        }
        max_flow += path_flow;
    }
    return max_flow;
}

int main(void) {
    printf("Enter number of nodes: ");
    if (scanf("%d", &node_count) != 1)
        return 1;

    printf("Enter capacity matrix (use 0 for no edge):\n");
    for (int i = 0; i < node_count; i++)
        for (int j = 0; j < node_count; j++)
            if (scanf("%d", &capacity[i][j]) != 1)
                return 1;

    int source, sink;
    printf("Enter source node: ");
    if (scanf("%d", &source) != 1)
        return 1;
    printf("Enter sink node: ");
    if (scanf("%d", &sink) != 1)
        return 1;

    int max_flow = edmonds_karp(source, sink);
    printf("Max flow: %d\n", max_flow);
    return 0;
}

/*
Compile with:
    gcc edmonds_karp.c -o edmonds_karp
*/
