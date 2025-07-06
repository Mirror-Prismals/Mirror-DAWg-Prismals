#include <stdio.h>
#include <string.h>
#include <limits.h>

#define MAXV 105
#define MAXE 2000

struct Edge {
    int to;
    int rev;
    int cap;
    int next;
};

struct Graph {
    int head[MAXV];
    struct Edge edges[MAXE];
    int ecnt;
};

void init_graph(struct Graph *g) {
    for (int i = 0; i < MAXV; ++i) g->head[i] = -1;
    g->ecnt = 0;
}

void add_edge(struct Graph *g, int u, int v, int cap) {
    struct Edge a = {v, g->ecnt + 1, cap, g->head[u]};
    g->edges[g->ecnt] = a;
    g->head[u] = g->ecnt++;

    struct Edge b = {u, g->ecnt - 1, 0, g->head[v]};
    g->edges[g->ecnt] = b;
    g->head[v] = g->ecnt++;
}

int level[MAXV];
int ptr[MAXV];

int bfs(struct Graph *g, int s, int t) {
    static int q[MAXV];
    int front = 0, back = 0;
    memset(level, -1, sizeof(level));
    level[s] = 0;
    q[back++] = s;
    while (front < back) {
        int v = q[front++];
        for (int e = g->head[v]; e != -1; e = g->edges[e].next) {
            if (g->edges[e].cap > 0 && level[g->edges[e].to] == -1) {
                level[g->edges[e].to] = level[v] + 1;
                q[back++] = g->edges[e].to;
            }
        }
    }
    return level[t] != -1;
}

int dfs(struct Graph *g, int v, int t, int pushed) {
    if (pushed == 0) return 0;
    if (v == t) return pushed;
    for (int *e = &ptr[v]; *e != -1; *e = g->edges[*e].next) {
        struct Edge *ed = &g->edges[*e];
        if (level[ed->to] != level[v] + 1 || ed->cap <= 0) continue;
        int tr = dfs(g, ed->to, t, pushed < ed->cap ? pushed : ed->cap);
        if (tr == 0) continue;
        ed->cap -= tr;
        g->edges[ed->rev].cap += tr;
        return tr;
    }
    return 0;
}

int dinic(struct Graph *g, int s, int t) {
    int flow = 0;
    while (bfs(g, s, t)) {
        memcpy(ptr, g->head, sizeof(int) * MAXV);
        while (1) {
            int pushed = dfs(g, s, t, INT_MAX);
            if (pushed == 0) break;
            flow += pushed;
        }
    }
    return flow;
}

int main(void) {
    struct Graph g;
    init_graph(&g);

    int n, m;
    if (scanf("%d %d", &n, &m) != 2) return 0;
    for (int i = 0; i < m; ++i) {
        int u, v, c;
        scanf("%d %d %d", &u, &v, &c);
        add_edge(&g, u, v, c);
    }
    int s, t;
    scanf("%d %d", &s, &t);

    int maxflow = dinic(&g, s, t);
    printf("%d\n", maxflow);
    return 0;
}
