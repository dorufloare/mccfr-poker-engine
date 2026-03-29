/*
    trying to figure out how many iterations i can shave off in the
    EHS evaluation without losing too much accuracy
*/

#include "evaluation.h"
#include "decision_state.h"
#include "info_set.h"
#include "utils.h"
#include "cards.h"
#include "infoset_key.h"
#include "optimal_ehs_iterations.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace experiments {

inline uint32_t deterministic_seed(const mccfr::InfoSetKey& k) {
    uint64_t mix = k.hole;
    mix ^= (k.board + 0x9e3779b97f4a7c15ULL + (mix << 6) + (mix >> 2));
    mix ^= (static_cast<uint64_t>(k.street) << 8);
    mix ^= (static_cast<uint64_t>(k.position) << 16);
    mix ^= (static_cast<uint64_t>(k.street_actions) << 24);
    mix ^= (static_cast<uint64_t>(k.to_call) << 32);
    uint32_t seed = static_cast<uint32_t>(mix ^ (mix >> 32));
    return random_utils::init_rng(seed == 0 ? 1u : seed);
}

inline uint8_t ehs_bucket(const mccfr::InfoSetKey& k) {
    state::DecisionState s{};
    s.hole_cards = k.hole;
    s.board_cards = k.board;
    uint32_t rng = deterministic_seed(k);
    int ehs_sims = 16;
    if (k.street == static_cast<uint8_t>(state::Street::FLOP))
        ehs_sims = 32;
    else if (k.street == static_cast<uint8_t>(state::Street::TURN))
        ehs_sims = 64;
    const float ehs = std::clamp(eval::evaluate_hand_EHS(s, rng, ehs_sims), 0.0f, 1.0f);
    return static_cast<uint8_t>(std::min(9, static_cast<int>(ehs * 10.0f)));
}

void try_ehs_iterations(const state::Street curr_street, const int& ehs_iterations) {
    const int opt_iterations = 1000;
    const int nr_states = 15;

    uint32_t rng = random_utils::init_rng(12345);

    int perfect_matches = 0; 
    int close_matches = 0; // EHS buckets within 1 of each other

    for (int i = 0; i < nr_states; ++i) {
        state::DecisionState curr_state;
        curr_state.street = curr_street;
        curr_state.position = state::Position::IN_POSITION;
        curr_state.hole_cards = cards::draw_random_cards(rng, 0, 2);
        curr_state.board_cards = 0;
        curr_state.to_call = random_utils::fast_rand(rng) % 20;
        curr_state.stack_self = 50;
        curr_state.stack_opp = 50;

        switch(curr_street) {
            case state::Street::PREFLOP:
                break;
            case state::Street::FLOP:
                curr_state.board_cards = cards::draw_random_cards(rng, curr_state.hole_cards, 3);
                break;
            case state::Street::TURN:
                curr_state.board_cards = cards::draw_random_cards(rng, curr_state.hole_cards, 4);
                break;
            case state::Street::RIVER:
                curr_state.board_cards = cards::draw_random_cards(rng, curr_state.hole_cards, 5);
                break;
            default:
                break;
        }

        float actual_ehs = eval::evaluate_hand_EHS(curr_state, rng, opt_iterations);
        float try_ehs = eval::evaluate_hand_EHS(curr_state, rng, ehs_iterations);

        int actual_bucket = static_cast<int>(actual_ehs * 10.0f);
        int try_bucket = static_cast<int>(try_ehs * 10.0f);

        if (actual_bucket == try_bucket) {
            ++perfect_matches;
        } else if (std::abs(actual_bucket - try_bucket) <= 1) {
            ++close_matches;
        }
    }

    float perfect_match_rate = static_cast<float>(perfect_matches) / nr_states;
    float close_match_rate = static_cast<float>(close_matches) / nr_states;
    float fail_rate = 1.0f - perfect_match_rate - close_match_rate;

    std::cout << "-------------------------------\n";
    std::cout << "EHS iterations: " << ehs_iterations << '\n';
    std::cout << "perfect match rate: " << perfect_match_rate << '\n';
    std::cout << "close match rate: " << close_match_rate << '\n';
    std::cout << "fail rate: " << fail_rate << '\n';
}

void run_ehs_iterations_experiment() {
    //PREFLOP
    std::cout << "PREFLOP:\n";

    const std::vector<int> possible_iterations = {4, 8, 16, 32, 64, 128};

    for (int iters : possible_iterations) {
        try_ehs_iterations(state::Street::PREFLOP, iters);
    }

    //FLOP
    std::cout << "\nFLOP:\n";
    for (int iters : possible_iterations) {
        try_ehs_iterations(state::Street::FLOP, iters);
    }

    //TURN
    std::cout << "\nTURN:\n";
    for (int iters : possible_iterations) {
        try_ehs_iterations(state::Street::TURN, iters);
    }

    //RIVER
    std::cout << "\nRIVER:\n";
    for (int iters : possible_iterations) {
        try_ehs_iterations(state::Street::RIVER, iters);
    }
}

}   // namespace experiments