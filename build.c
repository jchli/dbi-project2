#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "tree.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("usage: %s <num keys> <num probes> <list of fanout parameters...>", argv[0]);
    }
    
    int32_t num_keys   = atoi(argv[1]);
    int32_t num_probes = atoi(argv[2]);
    int32_t num_levels = argc - 3;
    int32_t fanouts[num_levels];
    size_t  i;
    for (i = 0; i != num_levels; i++) {
        fanouts[i] = atoi(argv[i+3]);
    }

    partition_tree tree;
    init_partition_tree(num_keys, num_levels, fanouts, &tree);
    /* printf("building partition tree with %d keys, %d probes, %d levels\n", */
    /*        num_keys, num_probes, num_levels); */

    print_partition_tree(&tree);


    int32_t lower = INT32_MIN;
    int32_t upper = INT32_MAX;
    int32_t probe = 200;
    int32_t array[10] = { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18 };
    
    int32_t length = sizeof(array) / sizeof(array[0]);
    printf ("index:%d\n", length);
    binary_search_array(array, length, probe, &lower, &upper);
    printf ("lower:%d, upper:%d\n", lower, upper);

    return 0;
}
