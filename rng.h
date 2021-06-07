#ifndef RNG_H
#define RNG_H
#include <stdint.h>
#include <stdlib.h>
#include "david_macros.h"
#include "error_handling.h"
typedef struct RngState {
    uint64_t state;
    uint64_t inc;
} RngState;

static inline
uint32_t
rng_random32(Nonnull(RngState*) rng) {
    auto oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t) ( ((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

static inline
uint32_t
stateless_random32(uint64_t state){
    state *= 6364136223846793005ULL;
    state += 123781241ULL;
    uint32_t xorshifted = (uint32_t) ( ((state >> 18u) ^ state) >> 27u);
    uint32_t rot = state >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

static inline
void
seed_rng_fixed(Nonnull(RngState*) rng, uint64_t initstate, uint64_t initseq) {
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    rng_random32(rng);
    rng->state += initstate;
    rng_random32(rng);
    }


#ifdef LINUX
#include <sys/random.h>
#endif
static inline
void
seed_rng(Nonnull(RngState*) rng) {
    _Static_assert(sizeof(unsigned long long) == sizeof(uint64_t), "");
    // spurious warnings on some platforms about unsigned long longs and unsigned longs,
    // so, use the unsigned long long.
    unsigned long long initstate;
    unsigned long long initseq;
#ifdef DARWIN
    arc4random_buf(&initstate, sizeof(initstate));
    arc4random_buf(&initseq, sizeof(initseq));
#elif defined(LINUX)
    // Too lazy to check for error.  Man page says we can't get
    // interrupted by signals for such a small buffer. and we're
    // not requesting nonblocking.
    ssize_t read;
    read = getrandom(&initstate, sizeof(initstate), 0);
    read = getrandom(&initseq, sizeof(initseq), 0);
    (void)read;
#else
    // Idk how to get random numbers on windows.
    // Just use the intel instruction.
    __builtin_ia32_rdseed64_step(&initstate);
    __builtin_ia32_rdseed64_step(&initseq);
#endif
    seed_rng_fixed(rng, initstate, initseq);
    return;
    }

// from
// https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
static inline
force_inline
uint32_t
fast_reduce(uint32_t x, uint32_t N){
    return ((uint64_t)x * (uint64_t)N) >> 32;
}

static inline
uint32_t
bounded_random(Nonnull(RngState*) rng, uint32_t bound) {
    uint32_t threshold = -bound % bound;
    // bounded loop to catch unitialized rng errors
    for(size_t i = 0 ; i < 10000; i++) {
        uint32_t r = rng_random32(rng);
        if (r >= threshold){
            auto temp = fast_reduce(r, bound);
            // auto temp = r % bound;
            return temp;
            }
        }
    // will assert(0) in debug
    // gcc will correctly convert the for to an infinite
    // loop in release, but clang can't figure it out.
    unreachable();
    }

// specializations for common numbers *much* faster
static inline
uint32_t
rng_bounded8(Nonnull(RngState*)rng){
    return rng_random32(rng) & 7u;
    }

static inline
uint32_t
rng_bounded4(Nonnull(RngState*)rng){
    return rng_random32(rng) & 3u;
    }

static inline
uint32_t
rng_bounded2(Nonnull(RngState*)rng){
    return rng_random32(rng) & 1u;
    }

static inline
uint32_t
rng_bounded_pow2(Nonnull(RngState*)rng, uint32_t bound){
    return rng_random32(rng) & (bound - 1u);
    }

static inline
uint32_t
random_range(Nonnull(RngState*) rng, uint32_t min, uint32_t max){
    return bounded_random(rng, max-min) + min;
    }

static inline
uint32_t
random_index(Nonnull(RngState*) rng, Nonnull(const uint32_t*) weights, uint32_t count){
    uint32_t total_weight = 0;
    for (uint32_t i = 0; i < count; i++){
        total_weight += weights[i];
        }
    auto roll = bounded_random(rng, total_weight);
    for (uint32_t i = 0, sum = 0; i < count; i++){
        sum += weights[i];
        if (sum > roll){
            return i;
            }
        }
    unreachable();
    }

static inline
uint32_t
random_int(Nonnull(RngState*) rng, uint32_t n){
    return bounded_random(rng, n)+1;
    }

static inline
bool
random_chance(Nonnull(RngState*) rng, uint32_t weight_happen, uint32_t total_weight){
    assert(weight_happen <= total_weight);
    return bounded_random(rng, total_weight) < weight_happen;
    }

#endif
