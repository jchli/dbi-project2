#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "tree.h"
#include "random.h"

// allocates memory aligned at 16-byte boundary
#define ALIGNED_ALLOC(ptr, size) posix_memalign((void **) (&(ptr)), 16, size)

int32_t num_keys_at_level(size_t level, int32_t *fanouts) {
    assert(level >= 0 && "level should be non-negative");

    int n = 1;
    size_t i;
    for (i = 0; i < level; i++)
        n *= fanouts[i];
    n *= (fanouts[level] - 1);

    return n;
}

int32_t max_num_keys(int32_t num_levels, int32_t *fanouts) {
    assert(num_levels >= 0 && "num_levels should be non-negative");

    if (num_levels == 0)
        return 0;
    
    return (fanouts[0]-1) + fanouts[0] * max_num_keys(num_levels-1, fanouts+1);
}

int32_t min_num_keys(int32_t num_levels, int32_t *fanouts) {
    assert(num_levels > 0 && "num_levels should be positive");

    // minimum: root has one key, left-most child all filled
    return max_num_keys(num_levels-1, fanouts+1) + 1;
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
        if (ALIGNED_ALLOC(tree->nodes[i],
                          sizeof(int32_t) * num_keys_at_level(i, fanouts))) {
            perror("posix_memalign");
            exit(EXIT_FAILURE);
        }
    }
    

    // populate the tree with randomly-generated integers
    rand32_t *gen = rand32_init(time(NULL));
    int32_t  *keys = generate_sorted_unique(k, gen);
    free(gen);

    // tail at each level
    size_t tails[num_levels];

    // boolean indicating whether each level just had a node filled
    int32_t filled[num_levels]; 

    for (i = 0; i < num_levels; i++) {
        tails[i] = 0;
        filled[i] = 0;
    }

    int32_t j;
    for (i = 0; i < k; i++) {
        for (j = num_levels - 1; j >= 0; j--) {
            if (filled[j]) {
                // go up one level and clear the filled bit
                filled[j] = 0;
                continue;
            } else {
                // insert key at this level
                tree->nodes[j][tails[j]] = keys[i];
                tails[j]++;
                if (tails[j] % (tree->fanouts[j] - 1) == 0) {
                    filled[j] = 1;
                }
                break;
            }
        }
    }

    // pad the rest of the tree with INT32_MAX
    for (i = 0; i < num_levels; i++) {
        for (j = tails[i]; j < num_keys_at_level(i, fanouts); j++)
            tree->nodes[i][j] = INT32_MAX;
    }

    free(keys);
}

void print_partition_tree(partition_tree *tree) {
    size_t i, j;
    for (i = 0; i != tree->num_levels; i++) {
        int32_t keys_at_level = num_keys_at_level(i, tree->fanouts);
        printf("level %zu: %d keys\n[", i, keys_at_level);

        for (j = 0; j < keys_at_level; j++) {
            if (tree->nodes[i][j] == INT32_MAX) {
                printf("MAX_INT...  ");
                break;
            }
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
