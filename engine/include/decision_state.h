#pragma once
#include <cstdint>
#include "cards.h"
#include "action.h"

namespace state {

enum class Street : uint8_t {
    PREFLOP = 0,
    FLOP    = 1,
    TURN    = 2,
    RIVER   = 3
};

enum class Position : uint8_t {
    OUT_OF_POSITION = 0,
    IN_POSITION     = 1
};

using Chips = uint8_t;

struct DecisionState {
    uint8_t hand_id;

    Street  street;
    Position position;
    action::ActionsMask action_mask;

    cards::CardsMask hole_cards;
    cards::CardsMask board_cards;

    Chips pot_size;
    Chips to_call;
    Chips stack_self;
    Chips stack_opp;
    
    uint8_t street_actions; // Number of actions taken on current street
};

int get_cards_left(const DecisionState& state, const int tableSize = 5) noexcept;

bool is_terminal_node(const DecisionState& state) noexcept;

bool is_chance_node(const DecisionState& state) noexcept;

DecisionState apply_action(const DecisionState& state,  action::ActionType action);

DecisionState deal_cards(DecisionState state, uint32_t& rng);

}; // namespace state