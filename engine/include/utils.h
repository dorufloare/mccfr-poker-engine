#pragma once
#include <cstdint>
#include <random>

namespace random_utils {

inline uint32_t fast_rand(uint32_t& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
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
