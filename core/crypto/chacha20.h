#ifndef CHACHA20_H
#define CHACHA20_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#define NULL ((void*)0)

struct chacha20_context
{
    uint32_t keystream32[16];
    uint32_t position;
    
    uint8_t key[32];
    uint8_t nonce[12];
    uint64_t counter;
    
    uint32_t state[16];
};

void chacha20_init_context(struct chacha20_context *ctx, uint8_t key[], uint8_t nonce[], uint64_t counter);
void chacha20_xor(struct chacha20_context *ctx, uint8_t *bytes, uint32_t n_bytes);

#endif