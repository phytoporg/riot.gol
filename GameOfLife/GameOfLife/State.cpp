#include "State.h"

#include <limits>
#include <cassert>
#include <algorithm>

namespace
{
    void 
    MaybeCreateNewNeighbors(
        Utility::AlignedMemoryPool<64>& memoryPool,
        const GameOfLife::SubGrid& subgrid,
        const GameOfLife::RectangularGrid& stateDimensions,
        GameOfLife::SubGridGraph& gridGraph,
        std::vector<GameOfLife::SubGrid>& subgrids
        )
    {
        using namespace GameOfLife;
        for (int i = 0; i < GameOfLife::AdjacencyIndex::MAX; ++i)
        {
            const auto Adjacency = static_cast<AdjacencyIndex>(i);
            if (subgrid.IsNextGenerationNeighbor(Adjacency))
            {
                const auto NeighborSubgridPosition =
                    SubGridGraph::GetNeighborPositionFromIndex(Adjacency);
                const int64_t NeighborMinX =
                    stateDimensions.XMin() + NeighborSubgridPosition.first * SubGrid::SUBGRID_WIDTH;
                const int64_t NeighborMinY =
                    stateDimensions.YMin() + NeighborSubgridPosition.second * SubGrid::SUBGRID_HEIGHT;

                const int64_t xMax = (stateDimensions.XMin() +  stateDimensions.Width()) - NeighborMinX;
                const int64_t yMax = (stateDimensions.YMin() + stateDimensions.Height()) - NeighborMinY;

                subgrids.emplace_back(
                    memoryPool, gridGraph,
                    NeighborMinX, std::min(SubGrid::SUBGRID_WIDTH, xMax),
                    NeighborMinY, std::min(SubGrid::SUBGRID_HEIGHT, yMax)
                    );
            }
        }
    }

    void 
    MaybeRetireSubgrid(
        uint32_t numCells,
        const GameOfLife::RectangularGrid& stateDimensions,
        const GameOfLife::SubGrid& subgrid,
        GameOfLife::SubGridGraph& gridGraph,
        GameOfLife::SubgridStorage& subgridStorage
        )
    {
        if (numCells)
        {
            return;
        }

        gridGraph.RemoveVertex(subgrid);
        subgridStorage.Remove(subgrid);
    }
}

namespace GameOfLife
{
    State::State(const std::vector<Cell>& initialState)
        : m_alignedPool((SubGrid::SUBGRID_WIDTH + 2) * (SubGrid::SUBGRID_HEIGHT + 2), 32)
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

        for (const auto& state : initialState)
        {
            const int64_t SubgridX = (state.X - m_xMin) / SubGrid::SUBGRID_WIDTH;
            const int64_t SubgridY = (state.Y - m_yMin) / SubGrid::SUBGRID_HEIGHT;

            const int64_t SubgridMinX = m_xMin + SubgridX * SubGrid::SUBGRID_WIDTH;
            const int64_t SubgridMinY = m_yMin + SubgridY * SubGrid::SUBGRID_HEIGHT;

            SubGrid* pSubGrid = nullptr;
            if (!m_gridGraph.QueryVertex(std::make_pair(SubgridMinX, SubgridMinY), &pSubGrid))
            {
                SubGrid subgrid(
                    m_alignedPool, m_gridGraph,
                    SubgridMinX, std::min(SubGrid::SUBGRID_WIDTH, xMax - SubgridMinX + 1),
                    SubgridMinY, std::min(SubGrid::SUBGRID_HEIGHT, yMax - SubgridMinY + 1)
                    );
                subgrid.RaiseCell(state.X, state.Y);
                m_subgridStorage.Add(subgrid, m_gridGraph);
            }
            else
            {
                pSubGrid->RaiseCell(state.X, state.Y);
            }
        }

        if (m_subgridStorage.GetSize() == 1)
        {
            return;
        }

        //
        // TODO: What if we have border columns/rows here, too? On init?
        // May have to create new subgrids here, too.
        //
        PopulateAdjacencyInfo(m_subgridStorage.begin(), m_subgridStorage.end());
    }

    bool State::AdvanceGeneration()
    {
        std::vector<SubGrid> subgridsToAdd;
        for (auto it = m_subgridStorage.begin(); it != m_subgridStorage.end(); ++it)
        {
            auto& subgrid = it->second;
            const uint32_t NumCells = subgrid.AdvanceGeneration();

            MaybeCreateNewNeighbors(m_alignedPool, subgrid, *this, m_gridGraph, subgridsToAdd);
            MaybeRetireSubgrid(NumCells, *this, subgrid, m_gridGraph, m_subgridStorage);
        }

        const size_t NumAdded = subgridsToAdd.size();
        if (NumAdded)
        {
            m_subgridStorage.Add(subgridsToAdd, m_gridGraph);
            PopulateAdjacencyInfo(m_subgridStorage.begin(), m_subgridStorage.end());
        }

        return true;
    }

    void State::PopulateAdjacencyInfo(
        SubgridStorage::const_iterator begin,
        SubgridStorage::const_iterator end
        )
    {
        //
        // TODO: Only assign adjacency to neighbors whose
        // cells will impact one another.
        //
        const int64_t xMax = m_xMin + m_width;
        const int64_t yMax = m_yMin + m_height;

        for (auto it = begin; it != end; ++it)
        {
            const auto& subgrid = it->second;
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
                        if (neighborX > xMax)        { neighborX = m_xMin; }
                        else if (neighborX < m_xMin) { neighborX = m_xMin + (m_width / SubGrid::SUBGRID_WIDTH) * SubGrid::SUBGRID_WIDTH; }

                        int64_t neighborY = SubgridCoordinates.second + dy * SubGrid::SUBGRID_HEIGHT;
                        if (neighborY > yMax)        { neighborY = m_yMin; }
                        else if (neighborY < m_yMin) { neighborY = m_yMin + (m_height / SubGrid::SUBGRID_HEIGHT) * SubGrid::SUBGRID_HEIGHT; }

                        const auto NeighborCoordinates = std::make_pair(neighborX, neighborY);
                        SubGrid* pNeighbor;
                        if (m_gridGraph.QueryVertex(NeighborCoordinates, &pNeighbor))
                        {
                            const AdjacencyIndex Adjacency = 
                                SubGridGraph::GetIndexFromNeighborPosition(std::make_pair(dx, dy));
                            m_gridGraph.AddEdge(subgrid, *pNeighbor, Adjacency);
                        }
                    }
                }
            }
        }
    }
}
