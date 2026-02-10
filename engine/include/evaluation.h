#pragma once
#include "decision_state.h"
#include <cstdint>
#include "cards.h"

namespace eval {

float evaluate_showdown(cards::CardsMask my_hole, cards::CardsMask opp_hole, cards::CardsMask board) noexcept;
float evaluate_hand_monte_carlo(const state::DecisionState& state, uint32_t rng, int num_simulations = 1000);

}; // namespace eval