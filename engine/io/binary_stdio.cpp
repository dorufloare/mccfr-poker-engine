#include "reader.h"
#include "writer.h"
#include "decision_state.h"
#include "action.h"
#include <iostream>

class StdInReader : public Reader {
public:
    bool read(state::DecisionState& out) override {
        return static_cast<bool>(std::cin.read(reinterpret_cast<char*>(&out), sizeof(out)));
    }
};

class StdOutWriter : public Writer {
public:
    void write(const action::Action& action) override {
        std::cout.write(reinterpret_cast<const char*>(&action), sizeof(action));
        std::cout.flush();
    }
};
