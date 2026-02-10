#include "decision_state.h"
#include "action.h"
#include "bot.h"
#include "action.h"

#include <iostream>

void run_hand(Bot& bot, const state::DecisionState& state) {
    action::Action action = bot.decide(state);
    // apply action to engine logic

    std::cout << "Hand " << state.hand_id 
              << ": ";

    switch (action.type) {
        case action::ActionType::FOLD:  std::cout << "FOLD"; break;
        case action::ActionType::CALL:  std::cout << "CALL"; break;
        case action::ActionType::RAISE: std::cout << "RAISE " << action.amount; break;
    }

    std::cout << std::endl;
}
