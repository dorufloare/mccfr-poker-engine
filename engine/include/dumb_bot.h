#pragma once
#include "bot.h"
#include "action.h"

class DumbBot : public Bot {
public:
    action::Action decide(const state::DecisionState& state) noexcept override;
};
