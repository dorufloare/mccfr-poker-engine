#include "mccfr.h"
#include "cards.h"
#include "evaluation.h"
#include "utils.h"
#include "decision_state.h"

namespace mccfr {

float MCCFR::traverse(const state::DecisionState& state, float reach_self, float reach_opp, uint32_t& rng) {
    if (state::is_terminal(state)) {
        return get_terminal_evaluation_from_state(state, rng);
    }

    InfoSet& infoset = get_infoset(state);
    float strategy[action::ACTIONS];

    infoset.get_strategy(strategy, reach_self);

    int action_index = random_utils::sample_discrete(strategy, action::ACTIONS, rng);
    action::ActionType action = static_cast<action::ActionType>(action_index);

    state::DecisionState next_state = state::apply_action(state, action, rng);
    float value = -traverse(next_state, reach_opp, reach_self * strategy[action_index], rng);

    for (int i = 0; i < action::ACTIONS; ++i) {
        float regret = (i == action_index ? 0.f : -value);
        infoset.regret_sum[i] += regret * reach_opp;
    }

    return value;
}

InfoSet& MCCFR::get_infoset(const state::DecisionState& state) {
    InfoSetKey key = make_key(state);
    auto it = infosets.find(key);
    if (it == infosets.end()) {
        it = infosets.emplace(key, InfoSet(action::ACTIONS)).first;
    }
    return it->second;
}

inline InfoSetKey make_key(const state::DecisionState& s) {
    InfoSetKey key;
    key.hole = s.hole_cards;
    key.board = s.board_cards;
    key.street = static_cast<uint8_t>(s.street);
    key.position = static_cast<uint8_t>(s.position);
    return key;
}

std::string infoset_key_to_string(const InfoSetKey& key) {
    return "Hole: " + cards::mask_to_string(key.hole) +
           ", Board: " + cards::mask_to_string(key.board) +
           ", Street: " + std::to_string(static_cast<int>(key.street)) +
           ", Position: " + std::to_string(static_cast<int>(key.position));
}

// Samples random opponent hole cards and remaining board cards, evaluates the terminal value of the hand for the current player
inline float get_terminal_evaluation_from_state(const state::DecisionState& s, uint32_t& rng) {
    cards::CardsMask used_cards = s.hole_cards | s.board_cards;
    cards::CardsMask opp_hole = cards::draw_random_cards(rng, used_cards, 2);

    used_cards |= opp_hole;
    cards::CardsMask full_board = cards::draw_random_cards(rng, used_cards, state::get_cards_left(s, 5)) | s.board_cards;

    return eval::evaluate_showdown(s.hole_cards, opp_hole, full_board);
}   

}; // namespace mccfr
