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