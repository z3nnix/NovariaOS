#ifndef CHACHA20_RNG_H
#define CHACHA20_RNG_H

#include <stdint.h>

struct chacha20_rng
{
    int32_t keystream32[16];
    int32_t position;
    int8_t key[32];
    int8_t nonce[12];
    int counter;
    int32_t state[16];
};

void chacha20_rng_init(struct chacha20_rng *rng, uint64_t seed);
int32_t chacha20_rng_next32(struct chacha20_rng *rng);
int chacha20_rng_next64(struct chacha20_rng *rng);
void chacha20_rng_bytes(struct chacha20_rng *rng, int8_t *buffer, int32_t size);

#endif