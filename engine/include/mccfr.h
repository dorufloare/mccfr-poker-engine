#pragma once
#include <unordered_map>
#include "decision_state.h"
#include "info_set.h"

namespace mccfr {

using InfoSetKey = uint64_t;

class MCCFR {
public:
    float traverse(const state::DecisionState& state, float reach_self, float reach_opp, uint32_t& rng);
    std::unordered_map<InfoSetKey, InfoSet>& get_infosets() { return infosets; }
private:
    std::unordered_map<InfoSetKey, InfoSet> infosets;
    InfoSet& get_infoset(const state::DecisionState& state);
};

inline float get_terminal_evaluation_from_state(const state::DecisionState& s, uint32_t& rng);
inline InfoSetKey make_key(const state::DecisionState& s);

};  // namespace mccfr