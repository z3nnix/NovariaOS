#ifndef CHACHA20_RNG_H
#define CHACHA20_RNG_H

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

struct chacha20_rng
{
    uint32_t keystream32[16];
    uint32_t position;
    uint8_t key[32];
    uint8_t nonce[12];
    uint64_t counter;
    uint32_t state[16];
};

void chacha20_rng_init(struct chacha20_rng *rng, uint64_t seed);
uint32_t chacha20_rng_next32(struct chacha20_rng *rng);
uint64_t chacha20_rng_next64(struct chacha20_rng *rng);
void chacha20_rng_bytes(struct chacha20_rng *rng, uint8_t *buffer, uint32_t size);

#endif