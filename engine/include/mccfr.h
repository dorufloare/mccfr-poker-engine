#pragma once
#include <unordered_map>
#include "decision_state.h"
#include "info_set.h"
#include "infoset_key.h"
namespace mccfr {

class MCCFR {
public:
    float traverse(const state::DecisionState& state, float reach_self, float reach_opp, uint32_t& rng);
    std::unordered_map<InfoSetKey, InfoSet, InfoSetKeyHash>& get_infosets() { return infosets; }
private:
    std::unordered_map<InfoSetKey, InfoSet, InfoSetKeyHash> infosets;
    InfoSet& get_infoset(const state::DecisionState& state);
};

inline float get_terminal_evaluation_from_state(const state::DecisionState& s, uint32_t& rng);

};  // namespace mccfr