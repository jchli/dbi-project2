#pragma once

typedef struct partition_tree {
    int *fanouts;
    int **levels;
} partition_tree;

partition_tree *build_partition_tree(int k, int p, int fanouts[]);
