#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "tree.h"
#include "random.h"

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
    print_partition_tree(&tree);

    // initialize the lower and upper bound
    int32_t lower = INT32_MIN;
    int32_t upper = INT32_MAX;
    rand32_t *gen = rand32_init(time(NULL));
    int32_t  *probes = generate_sorted_unique(num_probes, gen);

    /*
    for (size_t i = 0; i < num_probes; i++) {
        printf("%d \n", probes[i]);
    } */

    for (size_t i = 0; i < num_probes; i++){
        lower = INT32_MIN;
        upper = INT32_MAX;
        binary_search_partition(&tree, probes[i], &lower, &upper);
        printf("%d is between (%d, %d]\n", probes[i], lower, upper);
    }

    destroy_partition_tree(&tree);


    return 0;
}
