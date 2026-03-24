#include "mccfr.h"
#include "cards.h"
#include "evaluation.h"
#include "utils.h"
#include "decision_state.h"
#include <iostream>

namespace mccfr {

bool InfoSetKey::operator==(const InfoSetKey& other) const {
    InfoSetKeyHash hasher;
    return hasher(*this) == hasher(other);
}

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

 size_t InfoSetKeyHash::operator()(const InfoSetKey& k) const noexcept {
        size_t seed = 0;
        cards::CardsMask combined = k.hole | k.board;
        
        auto call_chips_bucket = [](int to_call) {
            if (to_call == 0) return 0;
            if (to_call <= 3) return 1;
            if (to_call <= 10) return 2;
            return 3;
        };

        hash_combine(seed, std::hash<uint64_t>{}(eval::highest_pair(combined)));
        hash_combine(seed, std::hash<uint64_t>{}(eval::highest_card(combined)));
        hash_combine(seed, std::hash<uint8_t>{}(k.street));
        hash_combine(seed, std::hash<uint8_t>{}(k.position));
        hash_combine(seed, std::hash<uint8_t>{}(call_chips_bucket(k.to_call)));

        return seed;
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
    key.street_actions = s.street_actions;
    key.to_call = s.to_call;
    return key;
}

std::string infoset_key_to_string(const InfoSetKey& key) {
    return "Hole: " + cards::mask_to_string(key.hole) +
           ", Board: " + cards::mask_to_string(key.board) +
           ", Street: " + std::to_string(static_cast<int>(key.street)) +
           ", Position: " + std::to_string(static_cast<int>(key.position)) +
           ", Actions: " + std::to_string(static_cast<int>(key.street_actions)) +
           ", To Call: " + std::to_string(static_cast<int>(key.to_call));
}

// Evaluates terminal value using both players' known hole cards
inline float get_terminal_evaluation_from_state(const state::DecisionState& s, uint32_t& rng) {
    // If someone folded (pot_size == 0), the opponent folded
    // After perspective swap in apply_action, "self" is now the winner
    if (s.pot_size == 0) {
        return 1.f;  // We won because opponent folded
    }

    // Showdown — deal remaining board cards if needed
    cards::CardsMask used_cards = s.hole_cards | s.opp_hole_cards | s.board_cards;
    int cards_left = state::get_cards_left(s, 5);
    cards::CardsMask full_board = s.board_cards;
    if (cards_left > 0) {
        full_board |= cards::draw_random_cards(rng, used_cards, cards_left);
    }

    float result = eval::evaluate_showdown(s.hole_cards, s.opp_hole_cards, full_board);
    
    // Convert win/loss/tie into chip EV
    // result: 1.0 = we win, 0.0 = we lose, 0.5 = tie
    float pot = static_cast<float>(s.pot_size);
    if (result == 1.f) return pot;       // we win the pot
    if (result == 0.f) return -pot;      // we lose
    return 0.f;                          // tie
}   

}; // namespace mccfr
