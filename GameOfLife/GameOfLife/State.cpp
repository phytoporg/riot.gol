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

        // TEMP
        m_subgrids.reserve(100);

        for (const auto& state : initialState)
        {
            //
            // TODO: This is stupid and hideous but it's 1am so fix later.
            //
            const int64_t SubgridX = (state.X - m_xMin) / SUBGRID_WIDTH;
            const int64_t SubgridY = (state.Y - m_yMin) / SUBGRID_HEIGHT;

            const int64_t SubgridMinX = m_xMin + SubgridX * SUBGRID_WIDTH;
            const int64_t SubgridMinY = m_yMin + SubgridY * SUBGRID_HEIGHT;

            SubGrid* pSubGrid = nullptr;
            if (!m_gridGraph.QueryVertex(std::make_pair(SubgridMinX, SubgridMinY), &pSubGrid))
            {
                m_subgrids.emplace_back(
                    m_gridGraph,
                    SubgridMinX, std::min(SUBGRID_WIDTH,  xMax - SubgridMinX + 1), 
                    SubgridMinY, std::min(SUBGRID_HEIGHT, yMax - SubgridMinY + 1)
                    );

                auto& subGrid = m_subgrids.back();
                subGrid.RaiseCell(state.X, state.Y);
                m_gridGraph.AddVertex(subGrid);
            }
            else
            {
                pSubGrid->RaiseCell(state.X, state.Y);
            }
        }

        //
        // Populate adjacency info. TODO: Only assign adjacency to neighbors whose
        // cells will impact one another.
        //
        for (auto& subgrid : m_subgrids)
        {
            const auto SubgridCoordinates = std::make_pair(subgrid.XMin(), subgrid.YMin());
            for (int64_t dy = -1; dy < 2; ++dy)
            {
                for (int64_t dx = -1; dx < 2; ++dx)
                {
                    if (dx || dy)
                    {
                        const auto NeighborCoordinates =
                            std::make_pair(
                                SubgridCoordinates.first + dx * SUBGRID_WIDTH,
                                SubgridCoordinates.second + dy * SUBGRID_HEIGHT
                                );
                        SubGrid* pNeighbor;
                        if (m_gridGraph.QueryVertex(NeighborCoordinates, &pNeighbor))
                        {
                            m_gridGraph.AddEdge(SubgridCoordinates, NeighborCoordinates);
                        }
                    }
                }
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
