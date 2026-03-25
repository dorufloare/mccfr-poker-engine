#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include "decision_state.h"

namespace mccfr {

struct InfoSetKey {
    cards::CardsMask hole;
    cards::CardsMask board;
    uint8_t street;
    uint8_t position;
    uint8_t street_actions;
    uint8_t to_call;

    bool operator==(const InfoSetKey& other) const;
};

struct InfoSetKeyHash {
    static inline void hash_combine(size_t& seed, size_t value) noexcept {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    size_t operator()(const InfoSetKey& k) const noexcept;
};

InfoSetKey make_key(const state::DecisionState& s);
std::string infoset_key_to_string(const InfoSetKey& key);

} // namespace mccfr
