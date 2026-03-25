#include "mccfr.h"
#include "evaluation.h"
#include "decision_state.h"

namespace mccfr {

float MCCFR::traverse(const state::DecisionState& state, float reach_self, float reach_opp, uint32_t& rng) {
    if (state::is_terminal_node(state)) {
        return get_terminal_evaluation_from_state(state, rng);
    }
    if (state::is_chance_node(state)) {
        state::DecisionState next = state::deal_cards(state, rng);
        return traverse(next, reach_self, reach_opp, rng);
    }

    InfoSet& infoset = get_infoset(state);
    float strategy[action::ACTIONS];
    float action_values[action::ACTIONS] = {};

    // Get current regret-matching strategy
    infoset.get_current_strategy(strategy);

    // Zero out invalid actions and renormalize
    float norm = 0.f;
    for (int i = 0; i < action::ACTIONS; ++i) {
        if ((state.action_mask & (1 << i)) == 0) {
            strategy[i] = 0.f;
        } else {
            norm += strategy[i];
        }
    }
    if (norm > 0.f) {
        for (int i = 0; i < action::ACTIONS; ++i)
            if (state.action_mask & (1 << i)) strategy[i] /= norm;
    } else {
        int valid = 0;
        for (int i = 0; i < action::ACTIONS; ++i)
            if (state.action_mask & (1 << i)) valid++;
        for (int i = 0; i < action::ACTIONS; ++i)
            strategy[i] = (state.action_mask & (1 << i)) ? (1.f / valid) : 0.f;
    }

    infoset.accumulate_strategy(strategy, reach_self);

    float node_value = 0.f;

    for (int i = 0; i < action::ACTIONS; ++i) {
        if ((state.action_mask & (1 << i)) == 0) {
            action_values[i] = 0.f;
            continue;
        }
        action::ActionType action = static_cast<action::ActionType>(i);
        state::DecisionState next_state = state::apply_action(state, action);
        action_values[i] = -traverse(next_state, reach_opp, reach_self * strategy[i], rng);
        node_value += strategy[i] * action_values[i];
    }

    for (int i = 0; i < action::ACTIONS; ++i) {
        if (state.action_mask & (1 << i)) {
            float regret = action_values[i] - node_value;
            infoset.regret_sum[i] += regret * reach_opp;
        }
    }

    return node_value;
}

InfoSet& MCCFR::get_infoset(const state::DecisionState& state) {
    InfoSetKey key = make_key(state);
    auto it = infosets.find(key);
    if (it == infosets.end()) {
        it = infosets.emplace(key, InfoSet(action::ACTIONS)).first;
    }
    return it->second;
}

// Evaluates terminal value using both players' known hole cards
inline float get_terminal_evaluation_from_state(const state::DecisionState& s, uint32_t& rng) {
    // Fold terminal: after perspective swap in apply_action, "self" is winner.
    if (s.folded) {
        const float final_self = static_cast<float>(s.stack_self + s.pot_size);
        const float final_opp = static_cast<float>(s.stack_opp);
        return 0.5f * (final_self - final_opp);
    }

    // Showdown - deal remaining board cards if needed
    cards::CardsMask used_cards = s.hole_cards | s.opp_hole_cards | s.board_cards;
    int cards_left = state::get_cards_left(s, 5);
    cards::CardsMask full_board = s.board_cards;
    if (cards_left > 0) {
        full_board |= cards::draw_random_cards(rng, used_cards, cards_left);
    }

    float result = eval::evaluate_showdown(s.hole_cards, s.opp_hole_cards, full_board);

    const float pot = static_cast<float>(s.pot_size);
    float final_self = static_cast<float>(s.stack_self);
    float final_opp = static_cast<float>(s.stack_opp);

    if (result == 1.f) {
        final_self += pot;
    } else if (result == 0.f) {
        final_opp += pot;
    } else {
        final_self += 0.5f * pot;
        final_opp += 0.5f * pot;
    }

    return 0.5f * (final_self - final_opp);
}   

}; // namespace mccfr
