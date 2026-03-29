#pragma once

#include <cstdint>

#include "action.h"
#include "cards.h"
#include "decision_state.h"

namespace training_config {

inline constexpr int kIterations = 3000;
inline constexpr int kProgressPrintEvery = 100;

inline constexpr int kEhsSimsPreflop = 32;
inline constexpr int kEhsSimsFlop = 32;
inline constexpr int kEhsSimsTurn = 48;
inline constexpr int kEhsSimsRiver = 48;

inline constexpr uint8_t kInitialHandId = 1;
inline constexpr state::Street kInitialStreet = state::Street::PREFLOP;
inline constexpr state::Position kInitialPosition = state::Position::OUT_OF_POSITION;
inline constexpr state::Chips kInitialPotSize = 9;
inline constexpr state::Chips kInitialStackSelf = 50;
inline constexpr state::Chips kInitialStackOpp = 50;
inline constexpr state::Chips kInitialToCall = 3;
inline constexpr uint8_t kInitialStreetActions = 0;

inline constexpr cards::CardsMask kEmptyCardsMask = 0;
inline constexpr int kHeroHoleCardCount = 2;
inline constexpr int kOppHoleCardCount = 2;

inline constexpr action::ActionsMask kInitialActionMask =
    action::FOLD_MASK | action::CALL_MASK | action::BET_SMALL_MASK | action::BET_BIG_MASK;

inline constexpr float kTraverseReachSelf = 1.0f;
inline constexpr float kTraverseReachOpp = 1.0f;

} // namespace training_config
