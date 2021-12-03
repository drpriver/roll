#ifndef RNG_H
#define RNG_H
// size_t
#include <stddef.h>
// uint64_t, uint32_t
#include <stdint.h>

#ifdef __linux__
// for getrandom
#include <sys/random.h>
#endif

#ifdef __APPLE__
// for arc4random_buf
#include <stdlib.h>
#endif

#include <assert.h>

#ifdef __clang__
#pragma clang assume_nonnull begin
#endif


typedef struct RngState {
    uint64_t state;
    uint64_t inc;
} RngState;

//
// Produces a uniform random u32.
//
static inline
uint32_t
rng_random32(RngState* rng){
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t) ( ((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}


//
// Seeds the rng with the given values.
//
static inline
void
seed_rng_fixed(RngState* rng, uint64_t initstate, uint64_t initseq){
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    rng_random32(rng);
    rng->state += initstate;
    rng_random32(rng);
}


//
// Seeds the rng using os APIs when available.
// If not available, uses rdseed on x64.
//
static inline
void
seed_rng_auto(RngState* rng){
    _Static_assert(sizeof(unsigned long long) == sizeof(uint64_t), "");
    // spurious warnings on some platforms about unsigned long longs and unsigned longs,
    // so, use the unsigned long long.
    unsigned long long initstate;
    unsigned long long initseq;
#ifdef __APPLE__
    arc4random_buf(&initstate, sizeof(initstate));
    arc4random_buf(&initseq, sizeof(initseq));
#elif defined(__linux__)
    // Too lazy to check for error.  Man page says we can't get
    // interrupted by signals for such a small buffer. and we're
    // not requesting nonblocking.
    ssize_t read;
    read = getrandom(&initstate, sizeof(initstate), 0);
    read = getrandom(&initseq, sizeof(initseq), 0);
    (void)read;
#elif defined(__GNUC__) || defined(__clang__)
    // Idk how to get random numbers on windows.
    // Just use the intel instruction.
    //
    // Idk what the intrinsic on MSVC is.
    __builtin_ia32_rdseed64_step(&initstate);
    __builtin_ia32_rdseed64_step(&initseq);
#else
#error "Need to specify a way to seed an rng with this compiler or platform"
#endif
    seed_rng_fixed(rng, initstate, initseq);
}

// from
// https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
static inline
force_inline
uint32_t
fast_reduce(uint32_t x, uint32_t N){
    return ((uint64_t)x * (uint64_t)N) >> 32;
}

//
// Returns a random u32 in the range of [0, bound).
//
static inline
uint32_t
bounded_random(RngState* rng, uint32_t bound){
    uint32_t threshold = -bound % bound;
    // bounded loop to catch unitialized rng errors
    for(size_t i = 0 ; i < 10000; i++){
        uint32_t r = rng_random32(rng);
        if(r >= threshold){
            uint32_t temp = fast_reduce(r, bound);
            // uint32_t temp = r % bound;
            return temp;
        }
    }
    assert(0);
#if defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#else
    return 0;
#endif
}

#ifdef __clang__
#pragma clang assume_nonnull end
#endif

#endif
