#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
 * Simple 2D k-d tree implementation with
 * nearest neighbor and range search.
 */

typedef struct Point {
    double x, y;
} Point;

typedef struct KDNode {
    Point point;
    struct KDNode *left;
    struct KDNode *right;
} KDNode;

static KDNode *kd_insert(KDNode *node, Point p, int depth) {
    if (!node) {
        KDNode *n = (KDNode *)malloc(sizeof(KDNode));
        n->point = p;
        n->left = n->right = NULL;
        return n;
    }
    int axis = depth % 2;
    if ((axis == 0 && p.x < node->point.x) ||
        (axis == 1 && p.y < node->point.y))
        node->left = kd_insert(node->left, p, depth + 1);
    else
        node->right = kd_insert(node->right, p, depth + 1);
    return node;
}

static double sqr(double x) { return x * x; }

static double distance2(Point a, Point b) {
    return sqr(a.x - b.x) + sqr(a.y - b.y);
}

static void kd_nearest(KDNode *node, Point target, int depth,
                       KDNode **best, double *best_dist2) {
    if (!node) return;
    double d2 = distance2(node->point, target);
    if (d2 < *best_dist2) {
        *best_dist2 = d2;
        *best = node;
    }
    int axis = depth % 2;
    KDNode *first = (axis == 0 ? (target.x < node->point.x ? node->left : node->right)
                               : (target.y < node->point.y ? node->left : node->right));
    KDNode *second = (first == node->left) ? node->right : node->left;
    kd_nearest(first, target, depth + 1, best, best_dist2);
    double diff = axis == 0 ? target.x - node->point.x : target.y - node->point.y;
    if (sqr(diff) < *best_dist2) {
        kd_nearest(second, target, depth + 1, best, best_dist2);
    }
}

static void kd_range(KDNode *node, double xmin, double xmax,
                     double ymin, double ymax, int depth) {
    if (!node) return;
    if (node->point.x >= xmin && node->point.x <= xmax &&
        node->point.y >= ymin && node->point.y <= ymax) {
        printf("(%.2f, %.2f)\n", node->point.x, node->point.y);
    }
    int axis = depth % 2;
    if ((axis == 0 && xmin <= node->point.x) ||
        (axis == 1 && ymin <= node->point.y))
        kd_range(node->left, xmin, xmax, ymin, ymax, depth + 1);
    if ((axis == 0 && xmax >= node->point.x) ||
        (axis == 1 && ymax >= node->point.y))
        kd_range(node->right, xmin, xmax, ymin, ymax, depth + 1);
}

static void kd_free(KDNode *node) {
    if (!node) return;
    kd_free(node->left);
    kd_free(node->right);
    free(node);
}

int main(void) {
    Point points[] = {
        {2, 3}, {5, 4}, {9, 6}, {4, 7}, {8, 1}, {7, 2}
    };
    const int npoints = sizeof(points) / sizeof(points[0]);
    KDNode *root = NULL;
    for (int i = 0; i < npoints; ++i)
        root = kd_insert(root, points[i], 0);

    Point query = {9, 2};
    KDNode *best = NULL;
    double best_d2 = 1e30;
    kd_nearest(root, query, 0, &best, &best_d2);
    if (best)
        printf("Nearest to (%.2f, %.2f) is (%.2f, %.2f)\n",
               query.x, query.y, best->point.x, best->point.y);

    printf("\nPoints in range x:[4,9], y:[2,7]:\n");
    kd_range(root, 4, 9, 2, 7, 0);

    kd_free(root);
    return 0;
}

