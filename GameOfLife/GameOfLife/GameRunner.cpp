#include "GameRunner.h"

#include "State.h"

#include <cassert>

namespace GameOfLife
{
    //
    // Internal state representation
    //

    GameRunner::GameRunner(const InitialState& initialState) : m_spState(new State(initialState))
    {}

    bool GameRunner::Tick()
    {
        return m_spState->AdvanceGeneration();
    }

    const State& GameRunner::CurrentState() const
    {
        return *m_spState;
    }
}
