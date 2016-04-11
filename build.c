#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "tree.h"
#include "random.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("usage: %s <num keys> <num probes> <list of fanout parameters...>\n", argv[0]);
        return 1;
    }
    
    int32_t num_keys   = atoi(argv[1]);
    int32_t num_probes = atoi(argv[2]);

    if (num_keys < 0) {
        printf("error: number of keys should be positive\n");
        return 1;
    }

    if (num_probes < 0) {
        printf("error: number of probes should be positive\n");
        return 1;
    }
    
    int32_t num_levels = argc - 3;
    int32_t fanouts[num_levels];
    size_t  i;
    for (i = 0; i != num_levels; i++) {
        fanouts[i] = atoi(argv[i+3]);
    }

    partition_tree tree;
    init_partition_tree(num_keys, num_levels, fanouts, &tree);
    // print_partition_tree(&tree);

    rand32_t *gen = rand32_init(time(NULL));
    int32_t  *probes = generate(num_probes, gen);

    for (size_t i = 0; i < num_probes; i++){
        int32_t range = -1;
        binary_search_partition(&tree, probes[i], &range);
        printf("%d %d\n", probes[i], range);
    }

    destroy_partition_tree(&tree);

    return 0;
}
