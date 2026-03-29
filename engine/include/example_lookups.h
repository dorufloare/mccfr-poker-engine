#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "cards.h"
#include "decision_state.h"
#include "infoset_key.h"

namespace mccfr {
class MCCFR;
}

struct TestConfig {
	std::string description;
	cards::CardsMask hole = 0;
	cards::CardsMask board = 0;
	state::Chips stack_self = 50;
	state::Chips stack_opp = 50;
	state::Street street = state::Street::PREFLOP;
	state::Position position = state::Position::OUT_OF_POSITION;
	uint8_t street_actions = 0;
	uint8_t to_call = 0;

	mccfr::InfoSetKey to_key() const;
};

std::ostream& operator<<(std::ostream& os, const TestConfig& config);

std::vector<TestConfig> make_default_test_configs();

void print_example_lookups(mccfr::MCCFR& solver);
void print_example_lookups(mccfr::MCCFR& solver, const std::vector<TestConfig>& tests);
