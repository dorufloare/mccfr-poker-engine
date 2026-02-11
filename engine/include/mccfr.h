#pragma once
#include <unordered_map>
#include "decision_state.h"
#include "info_set.h"
#include "utils.h"
namespace mccfr {

struct InfoSetKey {
    cards::CardsMask hole;
    cards::CardsMask board;
    uint8_t street;
    uint8_t position;

    bool operator==(const InfoSetKey& other) const {
        return hole == other.hole &&
               board == other.board &&
               street == other.street &&
               position == other.position;
    }
};

struct InfoSetKeyHash {
    static inline void hash_combine(size_t& seed, size_t value) noexcept {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    size_t operator()(const InfoSetKey& k) const noexcept {
        size_t seed = 0;

        hash_combine(seed, std::hash<uint64_t>{}(k.hole));
        hash_combine(seed, std::hash<uint64_t>{}(k.board));
        hash_combine(seed, std::hash<uint8_t>{}(static_cast<uint8_t>(k.street)));
        hash_combine(seed, std::hash<uint8_t>{}(static_cast<uint8_t>(k.position)));

        return seed;
    }
};
class MCCFR {
public:
    float traverse(const state::DecisionState& state, float reach_self, float reach_opp, uint32_t& rng);
    std::unordered_map<InfoSetKey, InfoSet, InfoSetKeyHash>& get_infosets() { return infosets; }
private:
    std::unordered_map<InfoSetKey, InfoSet, InfoSetKeyHash> infosets;
    InfoSet& get_infoset(const state::DecisionState& state);
};

inline float get_terminal_evaluation_from_state(const state::DecisionState& s, uint32_t& rng);
inline InfoSetKey make_key(const state::DecisionState& s);
std::string infoset_key_to_string(const InfoSetKey& key);

};  // namespace mccfr