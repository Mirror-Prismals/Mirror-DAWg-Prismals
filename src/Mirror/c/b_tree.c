#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple B-Tree implementation supporting integer keys.
 * Order of the tree (minimum degree) is provided at runtime.
 * Bulk loading utility inserts all numbers contained in a file.
 */

typedef struct BTreeNode {
    int *keys;                // array of keys
    struct BTreeNode **child; // array of child pointers
    int n;                    // current number of keys
    int leaf;                 // is true when node is leaf
} BTreeNode;

typedef struct {
    BTreeNode *root;          // pointer to root node
    int t;                    // minimum degree
} BTree;

static BTreeNode *bt_create_node(BTree *tree, int leaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    node->leaf = leaf;
    node->n = 0;
    node->keys = (int *)malloc(sizeof(int) * (2 * tree->t - 1));
    node->child = (BTreeNode **)malloc(sizeof(BTreeNode *) * (2 * tree->t));
    return node;
}

static BTree *bt_create(int t) {
    BTree *tree = (BTree *)malloc(sizeof(BTree));
    tree->t = t;
    tree->root = bt_create_node(tree, 1);
    return tree;
}

static void bt_split_child(BTree *tree, BTreeNode *x, int i, BTreeNode *y) {
    BTreeNode *z = bt_create_node(tree, y->leaf);
    z->n = tree->t - 1;

    for (int j = 0; j < tree->t - 1; ++j)
        z->keys[j] = y->keys[j + tree->t];

    if (!y->leaf) {
        for (int j = 0; j < tree->t; ++j)
            z->child[j] = y->child[j + tree->t];
    }

    y->n = tree->t - 1;

    for (int j = x->n; j >= i + 1; --j)
        x->child[j + 1] = x->child[j];
    x->child[i + 1] = z;

    for (int j = x->n - 1; j >= i; --j)
        x->keys[j + 1] = x->keys[j];
    x->keys[i] = y->keys[tree->t - 1];

    x->n += 1;
}

static void bt_insert_nonfull(BTree *tree, BTreeNode *x, int k) {
    int i = x->n - 1;
    if (x->leaf) {
        while (i >= 0 && k < x->keys[i]) {
            x->keys[i + 1] = x->keys[i];
            i--;
        }
        x->keys[i + 1] = k;
        x->n += 1;
    } else {
        while (i >= 0 && k < x->keys[i])
            i--;
        i++;
        if (x->child[i]->n == 2 * tree->t - 1) {
            bt_split_child(tree, x, i, x->child[i]);
            if (k > x->keys[i])
                i++;
        }
        bt_insert_nonfull(tree, x->child[i], k);
    }
}

static void bt_insert(BTree *tree, int k) {
    BTreeNode *r = tree->root;
    if (r->n == 2 * tree->t - 1) {
        BTreeNode *s = bt_create_node(tree, 0);
        tree->root = s;
        s->child[0] = r;
        bt_split_child(tree, s, 0, r);
        bt_insert_nonfull(tree, s, k);
    } else {
        bt_insert_nonfull(tree, r, k);
    }
}

static BTreeNode *bt_search(BTreeNode *x, int k) {
    int i = 0;
    while (i < x->n && k > x->keys[i])
        i++;
    if (i < x->n && k == x->keys[i])
        return x;
    if (x->leaf)
        return NULL;
    return bt_search(x->child[i], k);
}

static void bt_traverse(BTreeNode *x, int depth) {
    for (int i = 0; i < x->n; ++i) {
        if (!x->leaf)
            bt_traverse(x->child[i], depth + 1);
        for (int d = 0; d < depth; ++d)
            printf("  ");
        printf("%d\n", x->keys[i]);
    }
    if (!x->leaf)
        bt_traverse(x->child[x->n], depth + 1);
}

static void bt_free(BTreeNode *x) {
    if (!x->leaf) {
        for (int i = 0; i <= x->n; ++i)
            bt_free(x->child[i]);
    }
    free(x->keys);
    free(x->child);
    free(x);
}

static void bt_destroy(BTree *tree) {
    bt_free(tree->root);
    free(tree);
}

static void bt_bulk_load(BTree *tree, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return;
    }
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        int k = atoi(line);
        bt_insert(tree, k);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <order> [file_with_keys]\n", argv[0]);
        return 1;
    }
    int order = atoi(argv[1]);
    if (order < 2) {
        fprintf(stderr, "Order must be >= 2\n");
        return 1;
    }

    BTree *tree = bt_create(order);

    if (argc >= 3)
        bt_bulk_load(tree, argv[2]);

    printf("B-Tree contents (in-order):\n");
    bt_traverse(tree->root, 0);

    // simple interactive loop
    char cmd[64];
    while (1) {
        printf("command (i <num>=insert, s <num>=search, q=quit): ");
        if (!fgets(cmd, sizeof(cmd), stdin))
            break;
        if (cmd[0] == 'q')
            break;
        else if (cmd[0] == 'i') {
            int val = atoi(cmd + 1);
            bt_insert(tree, val);
            printf("Inserted %d\n", val);
        } else if (cmd[0] == 's') {
            int val = atoi(cmd + 1);
            BTreeNode *res = bt_search(tree->root, val);
            if (res)
                printf("%d found in tree\n", val);
            else
                printf("%d not found\n", val);
        }
    }

    printf("Final tree:\n");
    bt_traverse(tree->root, 0);
    bt_destroy(tree);
    return 0;
}

