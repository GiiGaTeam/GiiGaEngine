# AGENTS.md — Source/Core/cppGOAP/

A **GOAP (Goal-Oriented Action Planning)** AI planner. GOAP is an AI planning technique where an
agent is given a desired goal state and a pool of actions (each with preconditions, effects, and a
cost); the planner searches the space of action sequences to produce a plan that transforms the
current world state into the goal state — commonly used for emergent game AI behavior.

Everything here lives in the `goap::` namespace (nested under `GiiGa`).

## Files

| File | Class(es) | Purpose |
|------|-----------|---------|
| `WorldState.h` | `struct WorldState` | A snapshot of "the world" as a map of named boolean variables (`vars_`). Data that `Action`s operate on and the `Planner` compares. Provides `setVariable`/`getVariable`/`hasKey`, goal-matching `meetsGoal`, and a `distanceTo` heuristic for A*. |
| `Action.h` | `class Action` | A single atomic operation: boolean preconditions (`operableOn` checks them) and effects (`actOn` applies them to a copied state), plus a numeric `cost()` used by the planner and a `name()`. |
| `Node.h` | `struct Node` | One point in the A*/Dijkstra search graph: a world state, its parent (for path reconstruction), accumulated cost `g_`, heuristic estimate `h_`, and the action that led here. `f() = g_ + h_`. Includes free `operator<` (sorts by F-cost) and `operator<<` debug printing. |
| `Planner.h` | `class Planner` | The core planner. Given a start state, a goal state, and an action pool, searches for a sequence of actions. Maintains A*-style `open_`/`closed_` lists; exposes `planA` (A* with heuristics) and `planD` (Dijkstra fallback); `plan()` dispatches to A*. Returns `std::vector<std::shared_ptr<Action>>` in stepwise order. |

## How it fits together

```
WorldState (start) --[Action pool]--> Planner.plan() --> [Action, Action, ...] --> WorldState (goal)
```

The `GOAP` submodule of the Python `GiiGaPy` bindings (see
[`../PySripting/py_module.h`](../PySripting/py_module.h)) exposes `WorldState`, `Action`, and
`Planner` to Python, so AI can be authored from scripts (see `TemplateProject/Assets/Scripts/`
for GOAP examples like `GOAPBrain.py`, `Agent.py`).
