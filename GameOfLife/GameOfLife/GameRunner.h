#pragma once

#include <vector>
#include <memory>

#include "Cell.h"
#include "State.h"
#include "InitialState.h"

namespace GameOfLife
{
    class GameRunner
    {
    public:
        GameRunner(const InitialState& state);
        ~GameRunner() = default;

        //
        // Advances to the next generation.
        //
        bool Tick();

        const State& CurrentState() const;

    private:
        GameRunner() = delete; // This the right thing, here?
        GameRunner(const GameRunner& other) = delete;
        GameRunner& operator=(const GameRunner& other) = delete;

        std::unique_ptr<State> m_spState;
    };
}