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

        for (const auto& state : initialState)
        {
            assert(state.IsAlive);
            if (state.X < xMin) { xMin = state.X; }
            if (state.X > xMax) { xMax = state.X; }
            if (state.Y > yMax) { yMax = state.Y; }
            if (state.Y > yMax) { yMax = state.Y; }

            uint64_t subgridX = state.X / SUBGRID_WIDTH;
            uint64_t subgridY = state.X / SUBGRID_HEIGHT;

            auto it =
                std::find_if(
                    m_subgrids.begin(),
                    m_subgrids.end(),
                    [subgridX, subgridY](const SubGrid& subGrid)
                    {
                        return subGrid.XMin() == subgridX &&
                               subGrid.YMin() == subgridY;
                    });
            if (it == m_subgrids.end())
            {
                m_subgrids.emplace_back(SubGrid(subgridX, SUBGRID_WIDTH, subgridY, SUBGRID_HEIGHT));
                m_subgrids.back().RaiseCell(state.X, state.Y);
            }
        }

        m_xMin   = xMin;
        m_width  = xMax - xMin + 1;
        m_yMin   = yMin;
        m_height = yMax - yMin + 1;
    }

    bool State::AdvanceGeneration()
    {
        // TODO
        return false;
    }

    const std::vector<SubGrid>& State::GetSubgrids() const
    {
        return m_subgrids;
    }
}
