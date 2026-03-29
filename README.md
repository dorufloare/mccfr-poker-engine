# Poker Engine Notes

I am not saving training data to disk yet on purpose. Right now I am trying to find a good speed vs accuracy balance, and the abstractions are still moving.

The biggest challenge in this project is hand bucketing. I am using EHS buckets and a few extra features to keep the state space manageable, but every bucket decision changes both training speed and strategy quality. Until those choices feel stable, I prefer fast iteration over persistence.

## About the engine

MCCFR (Monte Carlo Counterfactual Regret Minimization) is the training method used here.

Very roughly, each training iteration does this:
1. Simulate one hand path through the game tree.
2. At every decision, compare the chosen action's outcome with the outcomes of other legal actions.
3. Increase regret for actions that would have done better.
4. Update strategy to favor actions with lower cumulative regret over time.

After enough iterations, the average strategy in each infoset becomes less exploitable and more stable.

## What this project does

This is a C++ toy poker engine with MCCFR training, hand evaluation, and lookup examples.

The flow is:
1. Build initial decision states.
2. Train with MCCFR.
3. Query trained infosets with curated test cases.

## Quick start

From the engine directory:

```bash
./run.sh
```

Run tests:

```bash
./tests.sh
```

## Config constants (important)

Main runtime knobs are in include/training_config.h.

Most important constants:
- kIterations: training iterations
- kProgressPrintEvery: training progress print frequency
- kEhsSimsPreflop
- kEhsSimsFlop
- kEhsSimsTurn
- kEhsSimsRiver

The EHS simulation counts directly affect bucketing quality and runtime. If training feels noisy or too slow, adjust these first.

### EHS in one minute

EHS means Effective Hand Strength. In plain words: it estimates how often your current hand ends up winning by showdown once future cards are considered.

In this project, EHS is estimated with Monte Carlo simulations, then bucketed into coarse ranges for infoset abstraction. More EHS sims usually means more stable buckets, but also slower training.

## Sample lookup outputs

These are example prints from the current lookup catalog.

```text
------------------------------------------------------------
Description: Nut flush draw (AhKh on Qh-7h-2c)
State      : Hole: Ah Kh, Board: Qh 7h 2c, ToCall: 7, StackSelf: 50, StackOpp: 50
Visits     : 707
AvgStr     : FOLD: 8.21% | CALL: 42.59% | BET_SMALL: 12.91% | BET_BIG: 36.28%

------------------------------------------------------------
Description: Open-ended straight draw (98o on 7-6-2)
State      : Hole: 9c 8d, Board: 7s 6h 2c, ToCall: 7, StackSelf: 50, StackOpp: 50
Visits     : 10744
AvgStr     : FOLD: 0.13% | CALL: 91.97% | BET_SMALL: 0.46% | BET_BIG: 7.44%

------------------------------------------------------------
Description: Royal flush (AhKh on Qh-Jh-Th-2c-3d)
State      : Hole: Ah Kh, Board: Qh Jh Th 3d 2c, ToCall: 0, StackSelf: 50, StackOpp: 50
Visits     : 4407
AvgStr     : FOLD: 0.00% | CHECK: 0.00% | BET_SMALL: 6.87% | BET_BIG: 93.13%

------------------------------------------------------------
Description: Quads aces (AcAd on As-Ah-7c-2d-9s)
State      : Hole: Ad Ac, Board: As Ah 9s 7c 2d, ToCall: 0, StackSelf: 50, StackOpp: 50
Visits     : 18242
AvgStr     : FOLD: 0.00% | CHECK: 3.93% | BET_SMALL: 9.20% | BET_BIG: 86.88%

------------------------------------------------------------
Description: Full house (KcKs on Kh-7d-7s-2c-9h)
State      : Hole: Ks Kc, Board: Kh 9h 7s 7d 2c, ToCall: 0, StackSelf: 50, StackOpp: 50
Visits     : 18242
AvgStr     : FOLD: 0.00% | CHECK: 3.93% | BET_SMALL: 9.20% | BET_BIG: 86.88%

------------------------------------------------------------
Description: Air facing river bet (23o on K-Q-J-9-7)
State      : Hole: 3d 2c, Board: Kh Qc Js 9d 7h, ToCall: 7, StackSelf: 50, StackOpp: 50
Visits     : 25295
AvgStr     : FOLD: 85.48% | CALL: 0.26% | BET_SMALL: 12.16% | BET_BIG: 2.10%
```

## Add your own lookup datapoint

There is a commented example in src/main.cpp showing how to:
1. Get the default test vector.
2. Push a custom TestConfig.
3. Print with print_example_lookups(solver, custom_tests).

Default test configs live in src/example_lookups.cpp.

## File map

### Top level
- CMakeLists.txt: build setup
- run.sh: configure/build/run main executable
- tests.sh: configure/build/run tests

### include
- include/action.h: action enums and masks
- include/cards.h: card bitmask utilities and card construction helpers
- include/decision_state.h: game state definition and state transition API
- include/evaluation.h: hand strength and showdown evaluation API
- include/info_set.h: regret and average strategy storage
- include/infoset_key.h: infoset key shape and hash/equality API
- include/mccfr.h: MCCFR solver API
- include/optimal_ehs_iterations.h: experiment entrypoint declaration
- include/example_lookups.h: TestConfig and lookup print API
- include/training_config.h: central constants for training and EHS sims
- include/utils.h: random and utility helpers

### src
- src/main.cpp: training entrypoint and example lookup call
- src/mccfr.cpp: MCCFR traversal and updates
- src/info_set.cpp: strategy and regret accumulation logic
- src/infoset_key.cpp: key abstraction logic, EHS bucketing, hashing/equality
- src/decision_state.cpp: action application, terminal/chance checks, board dealing
- src/evalutaion.cpp: hand ranking, showdown comparison, EHS Monte Carlo
- src/example_lookups.cpp: default lookup catalog and formatted printing

### experiments
- experiments/optimal_ehs_iterations.cpp: EHS iteration sensitivity experiment

### tests
- tests/test_evaluation.cpp: hand evaluator correctness tests (category ordering, ties, kickers, edge cases)

## Current design tradeoff

The core tradeoff here is abstraction quality vs training throughput.

If you make bucketing more detailed, strategy quality can improve but training slows down and infoset count grows.
If you simplify buckets, training gets much faster but policy quality can get unstable in marginal spots.

This project is currently optimized for exploring that tradeoff quickly.
