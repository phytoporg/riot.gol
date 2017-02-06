#pragma once

#include <vector>
#include <memory>

#include "Cell.h"
#include "State.h"
#include "InitialState.h"

namespace GoLReference
{
    //
    // The "driver" class which advances the internal state.
    //
    class GameRunner
    {
    public:
        GameRunner(const InitialState& state);
        ~GameRunner() = default;

        //
        // Advances to the next generation.
        //
        bool Tick();

        // 
        // Retrieve a reference to the current state.
        // 
        // TODO: Dangerous to use this reference after calling Tick(). Need
        // to implement some kind of weakptr-like object which invalidates
        // the state when it's no longer current.
        //
        // For now, just be careful on the consuming end.
        //
        const State& CurrentState() const;

    private:
        GameRunner() = delete; // This the right thing, here?
        GameRunner(const GameRunner& other) = delete;
        GameRunner& operator=(const GameRunner& other) = delete;

        // TODO: Pimpl
        std::unique_ptr<State> m_spStates[2];
        State* m_pCurrentState;
    };
}