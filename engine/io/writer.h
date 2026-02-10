#pragma once
#include "action.h"

class Writer {
public:
    virtual void write(const action::Action& action) = 0;
    virtual ~Writer() = default;
};
