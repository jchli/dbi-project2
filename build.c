#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "tree.h"
#include "random.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("usage: %s <num keys> <num probes> <list of fanout parameters...>\n", argv[0]);
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


    rand32_t *gen = rand32_init(time(NULL));
    int32_t  *probes = generate_sorted_unique(num_probes, gen);

    /*
    for (size_t i = 0; i < num_probes; i++) {
        printf("%d \n", probes[i]);
    } */

    for (size_t i = 0; i < num_probes; i++){
        int32_t range = -1;
        binary_search_partition(&tree, probes[i], &range);
        printf("%d is in range %d\n", probes[i], range);
    }

    /*
    int32_t array[7] = {0,2,4,6,8,10,12};
    int32_t lower_index = 2;
    int32_t upper_index = 5;
    binary_search_array(array, 7, 10, &lower_index, &upper_index);
    printf("%d\n", upper_index);
    */

    destroy_partition_tree(&tree);


    return 0;
}
