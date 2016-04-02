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
    
    printf("building partition tree with %d keys, %d probes, %d levels\n",
           num_keys, num_probes, num_levels);
    return 0;
}
