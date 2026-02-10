#pragma once
#include "decision_state.h"
#include "action.h"

class Bot {
public:
    virtual action::Action decide(const state::DecisionState& state) noexcept = 0;
    virtual ~Bot() = default;
};
