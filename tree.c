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

    int32_t n = 1;
    size_t i;
    for (i = 0; i < num_levels; i++) {
        n *= (fanouts[i] - 1);
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

void destroy_partition_tree(partition_tree *tree) {
    free(tree->fanouts);
    size_t i;
    for (i = 0; i < tree->num_levels; i++) {
        free(tree->nodes[i]);
    }
    free(tree->nodes);
}
