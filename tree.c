#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#include <xmmintrin.h>
#include <emmintrin.h>
#include <pmmintrin.h>
#include <tmmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>
#include <ammintrin.h>
#include <immintrin.h>
#include <x86intrin.h>

#include "tree.h"
#include "random.h"

// allocates memory aligned at 16-byte boundary
#define ALIGNED_ALLOC(ptr, size) {                          \
        if (posix_memalign((void **) (&(ptr)), 16, size)) { \
            perror("posix_memalign");                       \
            exit(EXIT_FAILURE);                             \
        }                                                   \
    }

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

void binary_search_array(int32_t *array, int32_t length, int32_t probe, int32_t *lower_index, int32_t *upper_index){
    int32_t min = *lower_index;
    int32_t max = *upper_index + 1;
    int32_t middle = (max + min) / 2;
    while (max - min > 1) {
        if (array[middle] < probe){
            min = middle;
        } else {
            max = middle;
        }
        middle = (max + min) / 2;
    }
    // the probe is smaller than the smallest number in the array 
    if (probe <= array[*lower_index]) {
        *upper_index = *lower_index;
        *lower_index = *upper_index - 1;
    } else {
        *upper_index = max;
        *lower_index = *upper_index - 1;
    }
}

void binary_search_partition(partition_tree *tree, int32_t probe, int32_t *range) {
    int32_t height = tree->num_levels;
    int32_t *fanouts = tree->fanouts;
    int32_t **nodes = tree->nodes;
    // initialize the range 
    *range = 0;
    int32_t lower_index, upper_index;
    for (size_t i = 0; i < height; i++) {
        int32_t length = fanouts[i] - 1;
        int32_t *array = nodes[i];
        lower_index = *range * length;
        upper_index = lower_index + length - 1;
        binary_search_array(array, length, probe, &lower_index, &upper_index);
        // add the offset to the upper_index from the binary search for the range
        *range = upper_index + *range;
    }
}

void binary_search_array_simd(int32_t *array, int32_t length, int32_t probe, 
                              int32_t *lower_index, int32_t *upper_index) {
    __m128i p = _mm_shuffle_epi32(_mm_cvtsi32_si128(probe),
                                  _MM_SHUFFLE(0,0,0,0));
    int32_t res = 0; // result: offset of first array element greater than probe
    
    switch (length) {
    case 4: {
        __m128i dels = _mm_load_si128((__m128i *) &array[*lower_index]);
        __m128i cmp = _mm_cmpgt_epi32(p, dels);
        
        cmp = _mm_packs_epi32(cmp, _mm_setzero_si128());
        cmp = _mm_packs_epi16(cmp, _mm_setzero_si128());

        // apparently _bit_scan_reverse(0 or 1) = 0
        // so I'm shifting mask by 1
        int mask = _mm_movemask_epi8(cmp);
        res  = _bit_scan_reverse(mask << 1);

        /* printf("keys: %d %d %d %d\n", array[*lower_index], array[*lower_index+1], array[*lower_index+2], array[*lower_index+3]); */
        /* printf("mask: %0x\n", mask); */
        /* printf("res:  %0x\n", res); */
        break;
    }

    case 8: {
        __m128i dels_ABCD = _mm_load_si128((__m128i *) &array[*lower_index]);
        __m128i dels_EFGH = _mm_load_si128((__m128i *) &array[*lower_index+4]);
        
        __m128i cmp_ABCD = _mm_cmpgt_epi32(p, dels_ABCD);
        __m128i cmp_EFGH = _mm_cmpgt_epi32(p, dels_EFGH);
        
        __m128i cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        __m128i cmp     = _mm_packs_epi16(cmp_A2H, _mm_setzero_si128());

        int mask = _mm_movemask_epi8(cmp);
        res  = _bit_scan_reverse(mask << 1);
        
        break;
    }

    case 16: {
        __m128i dels_ABCD = _mm_load_si128((__m128i *) &array[*lower_index]);
        __m128i dels_EFGH = _mm_load_si128((__m128i *) &array[*lower_index+4]);
        __m128i dels_IJKL = _mm_load_si128((__m128i *) &array[*lower_index+8]);
        __m128i dels_MNOP = _mm_load_si128((__m128i *) &array[*lower_index+12]);
        
        __m128i cmp_ABCD = _mm_cmpgt_epi32(p, dels_ABCD);
        __m128i cmp_EFGH = _mm_cmpgt_epi32(p, dels_EFGH);
        __m128i cmp_IJKL = _mm_cmpgt_epi32(p, dels_IJKL);
        __m128i cmp_MNOP = _mm_cmpgt_epi32(p, dels_MNOP);
        
        __m128i cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        __m128i cmp_I2P = _mm_packs_epi32(cmp_IJKL, cmp_MNOP);
        __m128i cmp     = _mm_packs_epi16(cmp_A2H,  cmp_I2P);

        int mask = _mm_movemask_epi8(cmp);
        res  = _bit_scan_reverse(mask << 1);

        break;
    }

    default:
        assert(0 && "length should be one of 4, 8, 16");
    }

    if (res == 0) {
        *upper_index = *lower_index;
        *lower_index = *lower_index - 1;
    } else {
        *lower_index = *lower_index + res - 1;
        *upper_index = *lower_index + 1;
    }
}

void binary_search_partition_simd(partition_tree *tree, int32_t probe, int32_t *range) {
    int32_t height = tree->num_levels;
    int32_t *fanouts = tree->fanouts;
    int32_t **nodes = tree->nodes;
    *range = 0;
    int32_t lower_index, upper_index;
    for (size_t i = 0; i < height; i++) {
        int32_t length = fanouts[i] - 1;
        int32_t *array = nodes[i];
        lower_index = *range * length;
        upper_index = lower_index + length - 1;
        binary_search_array_simd(array, length, probe, 
                                 &lower_index, &upper_index);
        *range = upper_index + *range;
    }
}

// debugging
void print128_num(__m128i var) {
    uint32_t *val = (uint32_t*) &var;
    printf("Numerical: %u %u %u %u\n",
           val[0], val[1], val[2], val[3]);
}

// hard-coded version of binary search
// 4 probes at a time
void binary_search_partition_959(partition_tree *tree, int32_t num_probes, int32_t* probes, int32_t *ranges) {
    assert(tree->num_levels == 3);
    assert(tree->fanouts[0] == 9 && tree->fanouts[1] == 5 && tree->fanouts[2] == 9);

    // load keys at root level into registers
    register __m128i root_ABCD = _mm_load_si128((__m128i *) (tree->nodes[0]));
    register __m128i root_EFGH = _mm_load_si128((__m128i *) (tree->nodes[0] + 4));

    __m128i dels_ABCD, dels_EFGH, cmp_ABCD, cmp_EFGH, cmp_A2H, cmp;
    int mask, res1, res2, res3, res4;

    for (size_t i = 0; i + 3 < num_probes; i += 4) {
        __m128i p = _mm_load_si128((__m128i *) &probes[i]);
        register __m128i p1 = _mm_shuffle_epi32(p, _MM_SHUFFLE(0,0,0,0));
        register __m128i p2 = _mm_shuffle_epi32(p, _MM_SHUFFLE(1,1,1,1));
        register __m128i p3 = _mm_shuffle_epi32(p, _MM_SHUFFLE(2,2,2,2));
        register __m128i p4 = _mm_shuffle_epi32(p, _MM_SHUFFLE(3,3,3,3));

        // first (root) level
        // p1
        cmp_ABCD = _mm_cmpgt_epi32(p1, root_ABCD);
        cmp_EFGH = _mm_cmpgt_epi32(p1, root_EFGH);
    
        cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        cmp     = _mm_packs_epi16(cmp_A2H, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res1 = _bit_scan_reverse(mask << 1);

        // p2
        cmp_ABCD = _mm_cmpgt_epi32(p2, root_ABCD);
        cmp_EFGH = _mm_cmpgt_epi32(p2, root_EFGH);
    
        cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        cmp     = _mm_packs_epi16(cmp_A2H, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res2 = _bit_scan_reverse(mask << 1);

        // p3
        cmp_ABCD = _mm_cmpgt_epi32(p3, root_ABCD);
        cmp_EFGH = _mm_cmpgt_epi32(p3, root_EFGH);
    
        cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        cmp     = _mm_packs_epi16(cmp_A2H, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res3 = _bit_scan_reverse(mask << 1);

        // p4
        cmp_ABCD = _mm_cmpgt_epi32(p4, root_ABCD);
        cmp_EFGH = _mm_cmpgt_epi32(p4, root_EFGH);
    
        cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        cmp     = _mm_packs_epi16(cmp_A2H, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res4 = _bit_scan_reverse(mask << 1);
    
        // second level
        // p1
        dels_ABCD = _mm_load_si128((__m128i *) (&tree->nodes[1][res1*4]));
        cmp_ABCD  = _mm_cmpgt_epi32(p1, dels_ABCD);
        
        cmp = _mm_packs_epi32(cmp_ABCD, _mm_setzero_si128());
        cmp = _mm_packs_epi16(cmp, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res1 = res1 * 5 + _bit_scan_reverse(mask << 1);

        // p2
        dels_ABCD = _mm_load_si128((__m128i *) (&tree->nodes[1][res2*4]));
        cmp_ABCD  = _mm_cmpgt_epi32(p2, dels_ABCD);
        
        cmp = _mm_packs_epi32(cmp_ABCD, _mm_setzero_si128());
        cmp = _mm_packs_epi16(cmp, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res2 = res2 * 5 + _bit_scan_reverse(mask << 1);

        // p3
        dels_ABCD = _mm_load_si128((__m128i *) (&tree->nodes[1][res3*4]));
        cmp_ABCD  = _mm_cmpgt_epi32(p3, dels_ABCD);
        
        cmp = _mm_packs_epi32(cmp_ABCD, _mm_setzero_si128());
        cmp = _mm_packs_epi16(cmp, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res3 = res3 * 5 + _bit_scan_reverse(mask << 1);

        // p4
        dels_ABCD = _mm_load_si128((__m128i *) (&tree->nodes[1][res4*4]));
        cmp_ABCD  = _mm_cmpgt_epi32(p4, dels_ABCD);
    
        cmp = _mm_packs_epi32(cmp_ABCD, _mm_setzero_si128());
        cmp = _mm_packs_epi16(cmp, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res4 = res4 * 5 + _bit_scan_reverse(mask << 1);
    
        // third level
        // p1
        dels_ABCD = _mm_load_si128((__m128i *) (&tree->nodes[2][res1*8]));
        dels_EFGH = _mm_load_si128((__m128i *) (&tree->nodes[2][res1*8] + 4));

        cmp_ABCD = _mm_cmpgt_epi32(p1, dels_ABCD);
        cmp_EFGH = _mm_cmpgt_epi32(p1, dels_EFGH);
    
        cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        cmp     = _mm_packs_epi16(cmp_A2H, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res1 = res1 * 9 + _bit_scan_reverse(mask << 1);

        // p2
        dels_ABCD = _mm_load_si128((__m128i *) (&tree->nodes[2][res2*8]));
        dels_EFGH = _mm_load_si128((__m128i *) (&tree->nodes[2][res2*8] + 4));

        cmp_ABCD = _mm_cmpgt_epi32(p2, dels_ABCD);
        cmp_EFGH = _mm_cmpgt_epi32(p2, dels_EFGH);
    
        cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        cmp     = _mm_packs_epi16(cmp_A2H, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res2 = res2 * 9 + _bit_scan_reverse(mask << 1);
    
        // p3
        dels_ABCD = _mm_load_si128((__m128i *) (&tree->nodes[2][res3*8]));
        dels_EFGH = _mm_load_si128((__m128i *) (&tree->nodes[2][res3*8] + 4));

        cmp_ABCD = _mm_cmpgt_epi32(p3, dels_ABCD);
        cmp_EFGH = _mm_cmpgt_epi32(p3, dels_EFGH);
    
        cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        cmp     = _mm_packs_epi16(cmp_A2H, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res3 = res3 * 9 + _bit_scan_reverse(mask << 1);

        // p4
        dels_ABCD = _mm_load_si128((__m128i *) (&tree->nodes[2][res4*8]));
        dels_EFGH = _mm_load_si128((__m128i *) (&tree->nodes[2][res4*8] + 4));

        cmp_ABCD = _mm_cmpgt_epi32(p4, dels_ABCD);
        cmp_EFGH = _mm_cmpgt_epi32(p4, dels_EFGH);
    
        cmp_A2H = _mm_packs_epi32(cmp_ABCD, cmp_EFGH);
        cmp     = _mm_packs_epi16(cmp_A2H, _mm_setzero_si128());

        mask = _mm_movemask_epi8(cmp);
        res4 = res4 * 9 + _bit_scan_reverse(mask << 1);

        ranges[i+0] = res1;
        ranges[i+1] = res2;
        ranges[i+2] = res3;
        ranges[i+3] = res4;
    }
}

void init_partition_tree(int32_t k, int32_t *keys, int32_t num_levels, int32_t *fanouts,
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
    ALIGNED_ALLOC(tree->fanouts, sizeof(int32_t  ) * num_levels);
    ALIGNED_ALLOC(tree->nodes,   sizeof(int32_t *) * num_levels);

    size_t i;
    for (i = 0; i < num_levels; i++) {
        // allocate memory for each level of tree (represented as a single array)
        tree->fanouts[i] = fanouts[i];
        ALIGNED_ALLOC(tree->nodes[i],
                      sizeof(int32_t) * num_keys_at_level(i, fanouts));
    }

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

    // pad each level to the fullest with INT32_MAX
    for (i = 0; i < num_levels; i++) {
        int32_t nkeys = num_keys_at_level(i, fanouts);
        for (j = tails[i]; j < nkeys; j++)
            tree->nodes[i][j] = INT32_MAX;
    }
}

void print_partition_tree(partition_tree *tree) {
    size_t i, j;
    for (i = 0; i != tree->num_levels; i++) {
        int32_t keys_at_level = num_keys_at_level(i, tree->fanouts);
        printf("level %zu: %d keys\n[", i, keys_at_level);

        for (j = 0; j < keys_at_level; j++) {
            if (tree->nodes[i][j] == INT32_MAX) {
                printf("MAX, ");
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

