#include "State.h"

#include <limits>
#include <cassert>
#include <algorithm>

namespace GameOfLife
{
    State::State(const InitialState& initialState)
    {
        assert(!initialState.empty());
        int64_t xMin = std::numeric_limits<int64_t>::max();
        int64_t xMax = std::numeric_limits<int64_t>::min();
        int64_t yMin = std::numeric_limits<int64_t>::max();
        int64_t yMax = std::numeric_limits<int64_t>::min();

        // 
        // First set state dimensions to reign in subgrids which may otherwise
        // stretch out of bounds.
        //

        for (const auto& state : initialState)
        {
            if (state.X < xMin) { xMin = state.X; }
            if (state.X > xMax) { xMax = state.X; }
            if (state.Y < yMin) { yMin = state.Y; }
            if (state.Y > yMax) { yMax = state.Y; }
        }

        m_xMin   = xMin;
        m_width  = xMax - xMin + 1;
        m_yMin   = yMin;
        m_height = yMax - yMin + 1;

        for (const auto& state : initialState)
        {
            const int64_t SubgridX = (state.X - m_xMin) / SUBGRID_WIDTH;
            const int64_t SubgridY = (state.Y - m_yMin) / SUBGRID_HEIGHT;

            const int64_t SubgridMinX = m_xMin + SubgridX * SUBGRID_WIDTH;
            const int64_t SubgridMinY = m_yMin + SubgridY * SUBGRID_HEIGHT;

            auto it =
                std::find_if(
                    m_subgrids.begin(),
                    m_subgrids.end(),
                    [SubgridMinX, SubgridMinY](const SubGrid& subGrid)
                    {
                        return subGrid.XMin() == SubgridMinX &&
                               subGrid.YMin() == SubgridMinY;
                    });
            if (it == m_subgrids.end())
            {
                m_subgrids.emplace_back(
                    SubgridMinX, std::min(SUBGRID_WIDTH,  xMax - SubgridMinX + 1), 
                    SubgridMinY, std::min(SUBGRID_HEIGHT, yMax - SubgridMinY + 1)
                    );
                m_subgrids.back().RaiseCell(state.X, state.Y);
            }
            else
            {
                it->RaiseCell(state.X, state.Y);
            }
        }

    }

    bool State::AdvanceGeneration()
    {
        for (auto& subgrid : m_subgrids)
        {
            subgrid.AdvanceGeneration();
        }

        return true;
    }

    const std::vector<SubGrid>& State::GetSubgrids() const
    {
        return m_subgrids;
    }
}
