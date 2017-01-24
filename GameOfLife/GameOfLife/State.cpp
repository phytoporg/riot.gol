#include "State.h"

#include <limits>
#include <cassert>
#include <algorithm>

// TEMP
#include <iostream>

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
        m_width  = std::max(xMax - xMin + 1, SubGrid::SUBGRID_WIDTH);
        m_yMin   = yMin;
        m_height = std::max(yMax - yMin + 1, SubGrid::SUBGRID_HEIGHT);

        //
        // TODO: Implement proper subgrid storage.
        //
        m_subgrids.reserve(100);

        for (const auto& state : initialState)
        {
            const int64_t SubgridX = (state.X - m_xMin) / SubGrid::SUBGRID_WIDTH;
            const int64_t SubgridY = (state.Y - m_yMin) / SubGrid::SUBGRID_HEIGHT;

            const int64_t SubgridMinX = m_xMin + SubgridX * SubGrid::SUBGRID_WIDTH;
            const int64_t SubgridMinY = m_yMin + SubgridY * SubGrid::SUBGRID_HEIGHT;

            SubGrid* pSubGrid = nullptr;
            if (!m_gridGraph.QueryVertex(std::make_pair(SubgridMinX, SubgridMinY), &pSubGrid))
            {
                m_subgrids.emplace_back(
                    m_gridGraph,
                    SubgridMinX, std::min(SubGrid::SUBGRID_WIDTH,  xMax - SubgridMinX + 1), 
                    SubgridMinY, std::min(SubGrid::SUBGRID_HEIGHT, yMax - SubgridMinY + 1)
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

        if (m_subgrids.size() == 1)
        {
            return;
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
                        // 
                        // State is toroidal, so account for wrapping.
                        //

                        int64_t neighborX = SubgridCoordinates.first + dx * SubGrid::SUBGRID_WIDTH;
                        if (neighborX > xMax)      { neighborX = xMin; }
                        else if (neighborX < xMin) { neighborX = xMin + (m_width / SubGrid::SUBGRID_WIDTH) * SubGrid::SUBGRID_WIDTH; }

                        int64_t neighborY = SubgridCoordinates.second + dy * SubGrid::SUBGRID_HEIGHT;
                        if (neighborY > yMax)      { neighborY = yMin; }
                        else if (neighborY < yMin) { neighborY = yMin + (m_height / SubGrid::SUBGRID_HEIGHT) * SubGrid::SUBGRID_HEIGHT; }

                        const auto NeighborCoordinates = std::make_pair(neighborX, neighborY);
                        SubGrid* pNeighbor;
                        if (m_gridGraph.QueryVertex(NeighborCoordinates, &pNeighbor))
                        {
                            const AdjacencyIndex Adjacency = 
                                m_gridGraph.GetIndexFromNeighborPosition(std::make_pair(dx, dy));
                            m_gridGraph.AddEdge(subgrid, *pNeighbor, Adjacency);
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
