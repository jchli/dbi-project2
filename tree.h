#pragma once

typedef struct partition_tree {
    int32_t num_levels;
    int32_t *fanouts;
    int32_t **nodes;
} partition_tree;

/**
 * initializes and builds a partition tree with the given number 
 * of keys, levels, and fanout at each level
 * keys are generated with the provided random number generation code
 */
void init_partition_tree(int32_t k, int32_t num_levels, int32_t fanouts[],
                         partition_tree *tree);

/**
 * frees all resources associated with the given partition tree
 */
void destroy_partition_tree(partition_tree *tree);
