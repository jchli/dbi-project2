#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "tree.h"
#include "random.h"

#define NUM_EXPERIMENTS 100

// verifies that the resulting range of a probe is correct
void verify_probe(int32_t num_keys, int32_t *keys, int32_t probe, int32_t range);

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

    // generate keys
    rand32_t *gen  = rand32_init(time(NULL));
    int32_t  *keys = generate_sorted_unique(num_keys, gen);
    
    /* // these are for testing */
    /* int32_t *keys = malloc(num_keys * sizeof(int32_t)); */
    /* for (i = 0; i != num_keys; i++) */
    /*     keys[i] = i; */

    /* int32_t *probes = malloc(num_probes * sizeof(int32_t)); */
    /* for (i = 0; i != num_probes; i++) */
    /*     probes[i] = i; */

    // build the partition tree
    partition_tree tree;
    init_partition_tree(num_keys, keys, num_levels, fanouts, &tree);
    /* print_partition_tree(&tree); */

    double elapsed_times[NUM_EXPERIMENTS];
    for (int exp = 0; exp < NUM_EXPERIMENTS; exp++) {
        // generate probes
        int32_t  *probes = generate(num_probes, gen);

        clock_t start = clock();

        // binary search
        if (num_levels == 3 && fanouts[0] == 9 && fanouts[1] == 5 && fanouts[2] == 9) {
            // hard-coded 9-5-9 tree
            int32_t ranges[4];
            for (i = 0; i + 3 < num_probes; i += 4) {
                binary_search_partition_959(&tree, &probes[i], ranges);

                /* for (size_t j = 0; j < 4; j++) { */
                /*     verify_probe(num_keys, keys, probes[i+j], ranges[j]); */
                /*     printf("%d %d\n", probes[i+j], ranges[j]); */
                /* } */
            }
        } else {
            int32_t range = -1;
            for (i = 0; i < num_probes; i++){
                binary_search_partition_simd(&tree, probes[i], &range);

                /* // NOTE: comment these out when doing performance tests */
                /* verify_probe(num_keys, keys, probes[i], range); */
                /* printf("%d %d\n", probes[i], range); */
            }
        }

        clock_t end = clock();
    
        
        elapsed_times[exp] = (end - start)/(double)CLOCKS_PER_SEC * 1000;
        /* printf("elapsed time for phase 2: %.3f milliseconds.\n", elapsed_times[exp]); */

        free(probes);
    }

    double total_time = 0.0;
    for (i = 0; i < NUM_EXPERIMENTS; i++) {
        total_time += elapsed_times[i];
    }
    printf("average elapsed time: %.3f milliseconds\n", total_time / NUM_EXPERIMENTS);

    destroy_partition_tree(&tree);
    free(gen);
    free(keys);

    return 0;
}

// NOTE: branch left if probe key is the same as delimiter
void verify_probe(int32_t num_keys, int32_t *keys, int32_t probe, int32_t range) {
    int32_t i = 0;
    for (i = 0; i < num_keys; i++) {
        if (probe <= keys[i]) {
            if (range != i)
                printf("WARNING: incorrect probe result, range should be %d\n",i);
            return ;
        }
    }

    if (range != num_keys)
        printf("WARNING: incorrect probe result, range should be %d\n", num_keys);
}
