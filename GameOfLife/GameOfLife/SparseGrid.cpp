#include "SparseGrid.h"

#include <limits>
#include <cassert>
#include <algorithm>

namespace
{
    GameOfLife::SubGrid::CoordinateType
    GetNeighborCoordinates(
        const GameOfLife::SubGrid& subgrid,
        const GameOfLife::RectangularGrid& stateDimensions,
        const GameOfLife::SubGrid::CoordinateType neighborDelta
        )
    {
        // 
        // SparseGrid is toroidal, so account for wrapping.
        //

        const int64_t XMax = stateDimensions.XMin() + stateDimensions.Width();
        const int64_t YMax = stateDimensions.YMin() + stateDimensions.Height();

        const int64_t dx = neighborDelta.first;
        const int64_t dy = neighborDelta.second;

        int64_t neighborX = subgrid.GetCoordinates().first + dx * GameOfLife::SubGrid::SUBGRID_WIDTH;
        if (neighborX > XMax)        
        { 
            neighborX = stateDimensions.XMin(); 
        }
        else if (neighborX < stateDimensions.XMin()) {
            //
            // The divide and multiply might look redundant, but the divide serves
            // also to shave off any remainder.
            //
            neighborX =
                stateDimensions.XMin() +
                (stateDimensions.Width() / GameOfLife::SubGrid::SUBGRID_WIDTH) *
                GameOfLife::SubGrid::SUBGRID_WIDTH; 
        }

        int64_t neighborY = subgrid.GetCoordinates().second + dy * GameOfLife::SubGrid::SUBGRID_HEIGHT;
        if (neighborY > YMax)        
        { 
            neighborY = stateDimensions.YMin(); 
        }
        else if (neighborY < stateDimensions.YMin()) 
        { 
            neighborY = stateDimensions.YMin() + 
                (stateDimensions.Height() / GameOfLife::SubGrid::SUBGRID_HEIGHT) *
                GameOfLife::SubGrid::SUBGRID_HEIGHT; 
        }

        return std::make_pair(neighborX, neighborY);
    }

    GameOfLife::RectangularGrid
    GetNeighborBounds(
        const GameOfLife::SubGrid& subgrid,
        const GameOfLife::RectangularGrid& stateDimensions,
        const GameOfLife::SubGrid::CoordinateType neighborDelta
        )
    {
        const auto MinCoordinates = GetNeighborCoordinates(subgrid, stateDimensions, neighborDelta);

        const int64_t XMax = stateDimensions.XMin() + stateDimensions.Width();
        int64_t width = GameOfLife::SubGrid::SUBGRID_WIDTH;
        if (width + MinCoordinates.first > XMax)
        {
            width = XMax - MinCoordinates.first;
        }

        const int64_t YMax = stateDimensions.YMin() + stateDimensions.Height();
        int64_t height = GameOfLife::SubGrid::SUBGRID_HEIGHT;
        if (height + MinCoordinates.second > YMax)
        {
            height = YMax - MinCoordinates.second;
        }

        return GameOfLife::RectangularGrid(
            MinCoordinates.first,
            width,
            MinCoordinates.second,
            height
            );
    }

    void 
    MaybeCreateNewNeighbors(
        Utility::AlignedMemoryPool<64>& memoryPool,
        const GameOfLife::SubGrid& subgrid,
        const GameOfLife::SparseGrid& state,
        GameOfLife::SubGridGraph& gridGraph,
        std::vector<GameOfLife::SubGrid>& subgrids
        )
    {
        using namespace GameOfLife;
        for (int i = 0; i < GameOfLife::AdjacencyIndex::MAX; ++i)
        {
            const auto Adjacency = static_cast<AdjacencyIndex>(i);
            const auto NeighborOffset = SubGridGraph::GetNeighborPositionFromIndex(Adjacency);
            if (subgrid.IsNextGenerationNeighbor(Adjacency))
            {
                const auto SubgridBounds = GetNeighborBounds(subgrid, state, NeighborOffset);
                
                const SubGrid::CoordinateType SubgridCoords = std::make_pair(SubgridBounds.XMin(), SubgridBounds.YMin());
                auto it = std::find_if(subgrids.begin(), subgrids.end(), [&SubgridCoords](const SubGrid& sg)
                {
                    return sg.GetCoordinates() == SubgridCoords;
                }); 

                SubGrid* pNeighbor;
                if (it == subgrids.end())
                {
                    uint8_t* ppBuffers[2] = { memoryPool.Allocate(), memoryPool.Allocate() };
                    subgrids.emplace_back(
                        ppBuffers, gridGraph,
                        SubgridBounds.XMin(), SubgridBounds.Width(),
                        SubgridBounds.YMin(), SubgridBounds.Height(),
                        subgrid.GetGeneration()
                        );
                    pNeighbor = &subgrids.back();
                }
                else
                {
                    pNeighbor = &(*it);
                }

                pNeighbor->CopyBorder(subgrid, GetReflectedAdjacencyIndex(Adjacency));
            }
        }
    }

    void MaybeRetireSubgrid(
        uint32_t numCells,
        GameOfLife::SubGrid& subgrid,
        std::vector<GameOfLife::SubGrid>& subgrids
        )
    {
        if (!numCells && !subgrid.HasBorderCells())
        {
            subgrids.push_back(subgrid);
        }
    }
}

namespace GameOfLife
{
    SparseGrid::SparseGrid(const std::vector<Cell>& initialState)
        : m_alignedPool((SubGrid::SUBGRID_WIDTH + 2) * (SubGrid::SUBGRID_HEIGHT + 2), 32),
          m_generationCount(0)
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
                uint8_t* ppBuffers[2] = { m_alignedPool.Allocate(), m_alignedPool.Allocate() };
                SubGrid subgrid(
                    ppBuffers,
                    m_gridGraph,
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

        std::vector<SubGrid> subgridsToAdd;
        for (auto it = m_subgridStorage.begin(); it != m_subgridStorage.end(); ++it)
        {
            auto& subgrid = it->second;
            MaybeCreateNewNeighbors(m_alignedPool, subgrid, *this, m_gridGraph, subgridsToAdd);
        }

        const size_t NumAdded = subgridsToAdd.size();
        if (NumAdded)
        {
            m_subgridStorage.Add(subgridsToAdd, m_gridGraph);
        }

        PopulateAdjacencyInfo(m_subgridStorage.begin(), m_subgridStorage.end());
    }

    SparseGrid::~SparseGrid()
    {
        for (auto& subgrids : m_subgridStorage)
        {
            subgrids.second.Cleanup(m_alignedPool);
        }
    }

    bool SparseGrid::AdvanceGeneration()
    {
        std::vector<SubGrid> subgridsToAdd;
        std::vector<SubGrid> subgridsToRemove;
        for (auto it = m_subgridStorage.begin(); it != m_subgridStorage.end(); ++it)
        {
            auto& subgrid = it->second;
            const uint32_t NumCells = subgrid.AdvanceGeneration();

            MaybeCreateNewNeighbors(m_alignedPool, subgrid, *this, m_gridGraph, subgridsToAdd);
            MaybeRetireSubgrid(NumCells, subgrid, subgridsToRemove);
        }

        const size_t NumRemoved = subgridsToRemove.size();
        for (size_t i = 0; i < NumRemoved; i++)
        {
            auto& subgrid = subgridsToRemove[i];
            m_gridGraph.RemoveVertex(subgrid);
            subgrid.Cleanup(m_alignedPool);
            m_subgridStorage.Remove(subgrid);
        }

        const size_t NumAdded = subgridsToAdd.size();
        if (NumAdded)
        {
            m_subgridStorage.Add(subgridsToAdd, m_gridGraph);
            PopulateAdjacencyInfo(m_subgridStorage.begin(), m_subgridStorage.end());
        }
         
        m_generationCount++;

        return true;
    }

    void SparseGrid::PopulateAdjacencyInfo(
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
            for (int i = 0; i < AdjacencyIndex::MAX; i++)
            {
                const AdjacencyIndex Adjacency = static_cast<AdjacencyIndex>(i);
                const auto Delta = SubGridGraph::GetNeighborPositionFromIndex(Adjacency);
                const auto NeighborCoordinates = GetNeighborCoordinates(subgrid, *this, Delta);

                SubGrid* pNeighbor;
                if (m_gridGraph.QueryVertex(NeighborCoordinates, &pNeighbor))
                {
                    m_gridGraph.AddEdge(subgrid, *pNeighbor, Adjacency);
                }
            }
        }
    }
}
