#pragma once
#include "decision_state.h"
#include <cstdint>
#include "cards.h"

namespace eval {

int highest_pair(cards::CardsMask mask) noexcept;
int highest_card(cards::CardsMask mask) noexcept;

float evaluate_showdown_simple(cards::CardsMask my_hole, cards::CardsMask opp_hole, cards::CardsMask board) noexcept;

float evaluate_showdown(cards::CardsMask my_hole, cards::CardsMask opp_hole, cards::CardsMask board) noexcept;
float evaluate_hand_monte_carlo(const state::DecisionState& state, uint32_t rng, int num_simulations = 1000);

}; // namespace eval