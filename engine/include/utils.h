#pragma once
#include <cstdint>
#include <random>

namespace random_utils {

inline uint32_t fast_rand(uint32_t& state) noexcept {
    // xorshift32 — state must never be 0
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

inline uint32_t init_rng(uint32_t seed) noexcept {
    if (seed == 0) seed = 1;
    // run a few rounds to warm up
    for (int i = 0; i < 8; ++i) fast_rand(seed);
    return seed;
}

inline int sample_discrete(const float* probs, int n, uint32_t& rng) noexcept {
    float r = (fast_rand(rng) & 0xFFFFFF) / float(0x1000000);
    float acc = 0.0f;
    for (int i = 0; i < n; ++i) {
        acc += probs[i];
        if (r <= acc) return i;
    }
    return n - 1;
}

};  // namespace random_utils
