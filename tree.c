#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "tree.h"
#include "random.h"

// allocates memory aligned at 16-byte boundary
#define ALIGNED_ALLOC(ptr, size) posix_memalign((void **) (&(ptr)), 16, size)

int32_t max_num_keys(int32_t num_levels, int32_t *fanouts) {
    assert(num_levels > 0 && "there should be at least one level in the tree");

    int32_t n = 0, keys_at_level = 1;
    size_t i;
    for (i = 0; i != num_levels; i++) {
        keys_at_level *= (fanouts[i] - 1);
        n += keys_at_level;
    }
    return n;
}

int32_t min_num_keys(int32_t num_levels, int32_t *fanouts) {
    assert(num_levels > 0 && "there should be at least one level in the tree");

    // minimum: root has one key, left-most child all filled
    return (num_levels == 1) ? 
        1 : max_num_keys(num_levels-1, fanouts+1) + 1;
}

void init_partition_tree(int32_t k, int32_t num_levels, int32_t *fanouts,
                         partition_tree *tree) {
    if (k > max_num_keys(num_levels, fanouts)) {
        fprintf(stderr, "error: too many build keys for partition tree, maximum %d keys\n",
                max_num_keys(num_levels, fanouts));
        exit(EXIT_FAILURE);
    }

    if (k < min_num_keys(num_levels, fanouts)) {
        fprintf(stderr, "error: too few build keys for partition tree, minimum %d keys\n",
                min_num_keys(num_levels, fanouts));
        exit(EXIT_FAILURE);
    }

    tree->num_levels = num_levels;
    if (ALIGNED_ALLOC(tree->fanouts, sizeof(int32_t  ) * num_levels) ||
        ALIGNED_ALLOC(tree->nodes,   sizeof(int32_t *) * num_levels)) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    size_t i;
    for (i = 0; i < num_levels; i++) {
        // allocate memory for each level of tree (represented as a single array)
        tree->fanouts[i] = fanouts[i];
        if (ALIGNED_ALLOC(tree->nodes[i], sizeof(int32_t) * fanouts[i])) {
            perror("posix_memalign");
            exit(EXIT_FAILURE);
        }
    }
    
    rand32_t *gen = rand32_init(time(NULL));
    int32_t  *keys = generate_sorted_unique(k, gen);

    // TODO: populate tree

    free(keys);
}

int32_t num_keys_at_level(int32_t level, partition_tree *tree) {
    if (level < 0 || level >= tree->num_levels) {
        fprintf(stderr, "error: invalid level\n");
        return 0;
    }

    int32_t n = 1;
    size_t i;
    for (i = 0; i <= level; i++)
        n *= (tree->fanouts[i] - 1);
    return n;
}

void print_partition_tree(partition_tree *tree) {
    size_t i, j;
    int32_t keys_at_level = 1;
    for (i = 0; i != tree->num_levels; i++) {
        printf("level %zu: [", i);

        keys_at_level *= (tree->fanouts[i] - 1);
        for (j = 0; j < keys_at_level; j++) {
            if (tree->nodes[i][j] == INT32_MAX) break;
            printf("%d, ", tree->nodes[i][j]);
        }

        printf("\b\b]\n");
    }
}

void destroy_partition_tree(partition_tree *tree) {
    free(tree->fanouts);
    size_t i;
    for (i = 0; i != tree->num_levels; i++) {
        free(tree->nodes[i]);
    }
    free(tree->nodes);
}
