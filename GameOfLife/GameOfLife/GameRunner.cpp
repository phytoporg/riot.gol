#include "GameRunner.h"

#include "State.h"

#include <cassert>

namespace
{
    GameOfLife::State* 
    OtherBuffer(
        GameOfLife::State* pState,
        const std::unique_ptr<GameOfLife::State>& spFirst,
        const std::unique_ptr<GameOfLife::State>& spSecond
    )
    {
        return pState == spFirst.get() ? spSecond.get() : spFirst.get();
    }

    uint8_t CountNeighbors(const GameOfLife::State& state, int64_t x, int64_t y)
    {
        uint8_t neighbors = 0;

        for (int64_t dy = -1; dy <= 1; ++dy)
        {
            for (int64_t dx = -1; dx <= 1; ++dx)
            {
                if (!dx && !dy) 
                {
                    //
                    // Don't count self!
                    //
                    continue; 
                }

                if (state.GetCellState(x + dx, y + dy))
                {
                    neighbors++;
                }
            }
        }

        return neighbors;
    }
}

namespace GameOfLife
{
    //
    // Internal state representation
    //

    GameRunner::GameRunner(const InitialState& initialState) : m_pCurrentState(nullptr)
    {
        m_spStates[0].reset(new State(initialState));
        m_spStates[1].reset(
            new State(
                m_spStates[0]->XMin(),
                m_spStates[0]->Width(),
                m_spStates[0]->YMin(),
                m_spStates[0]->Height()
                )
        );

        m_pCurrentState = m_spStates[0].get();
    }

    bool GameRunner::Tick()
    {
        const int64_t xMin = m_pCurrentState->XMin();
        const int64_t yMin = m_pCurrentState->YMin();

        int64_t x = xMin;
        int64_t y = yMin;

        State* pOtherBuffer = OtherBuffer(m_pCurrentState, m_spStates[0], m_spStates[1]);
        for (; y < yMin + m_pCurrentState->Height(); ++y)
        {
            for (; x < xMin + m_pCurrentState->Width(); ++x)
            {
                const uint8_t NumNeighbors = CountNeighbors(*m_pCurrentState, x, y);
                if (m_pCurrentState->GetCellState(x, y))
                {
                    if (NumNeighbors < 2 || NumNeighbors > 3)
                    {
                        pOtherBuffer->KillCell(x, y);
                    }
                    else
                    {
                        pOtherBuffer->RaiseCell(x, y);
                    }
                }
                else
                {
                    if (NumNeighbors == 3)
                    {
                        pOtherBuffer->RaiseCell(x, y);
                    }
                    else
                    {
                        pOtherBuffer->KillCell(x, y);
                    }
                }
            }
        }

        //
        // Swap()
        //
        m_pCurrentState = OtherBuffer(m_pCurrentState, m_spStates[0], m_spStates[1]);

        return true;
    }

    const State& GameRunner::CurrentState() const
    {
        assert(m_pCurrentState);
        return *m_pCurrentState;
    }
}
