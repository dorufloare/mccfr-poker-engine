#pragma once
#include "decision_state.h"

class Reader {
public:
    virtual bool read(state::DecisionState& out) = 0;
    virtual ~Reader() = default;
};
