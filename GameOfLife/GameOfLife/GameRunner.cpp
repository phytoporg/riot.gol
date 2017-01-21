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

    // TODO: Work against *subgrids*
    /*
    bool GameRunner::Tick()
    {
        const int64_t xMin = m_pCurrentState->XMin();
        const int64_t yMin = m_pCurrentState->YMin();

        State* pOtherBuffer = OtherBuffer(m_pCurrentState, m_spStates[0], m_spStates[1]);
        for (int64_t y = yMin; y < yMin + m_pCurrentState->Height(); ++y)
        {
            for (int64_t x = xMin; x < xMin + m_pCurrentState->Width(); ++x)
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
        // Swap buffers
        //
        m_pCurrentState = pOtherBuffer;

        return true;
    }
    */

    bool GameRunner::Tick()
    {
        return m_spState->AdvanceGeneration();
    }

    const State& GameRunner::CurrentState() const
    {
        return *m_spState;
    }
}
