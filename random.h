#pragma once

#include <stdint.h>

typedef struct {
	size_t index;
	uint32_t num[625];
} rand32_t;

rand32_t *rand32_init(uint32_t x);

int32_t *generate_sorted_unique(size_t n, rand32_t *gen);
