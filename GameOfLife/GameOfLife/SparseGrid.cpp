#include "SparseGrid.h"

#include <Utility/AlignedMemoryPool.h>

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
        const int64_t XMax = stateDimensions.XMin() + stateDimensions.Width();
        const int64_t YMax = stateDimensions.YMin() + stateDimensions.Height();

        const int64_t dx = neighborDelta.first;
        const int64_t dy = neighborDelta.second;

        const int64_t SubgridMinX = subgrid.GetCoordinates().first;
        const int64_t SubgridMinY = subgrid.GetCoordinates().second;

        // 
        // SparseGrid is toroidal, so account for wrapping.
        //
        int64_t neighborX = SubgridMinX + dx * GameOfLife::SubGrid::SUBGRID_WIDTH;
        if (neighborX < stateDimensions.XMin())
        {
            neighborX = XMax - GameOfLife::SubGrid::SUBGRID_WIDTH;
        }
        else if (neighborX == XMax)
        {
            neighborX = stateDimensions.XMin();
        }

        int64_t neighborY = SubgridMinY + dy * GameOfLife::SubGrid::SUBGRID_HEIGHT;
        if (neighborY < stateDimensions.YMin())
        {
            neighborY = YMax - GameOfLife::SubGrid::SUBGRID_HEIGHT;
        }
        else if (neighborY == YMax)
        {
            neighborY = stateDimensions.YMin();
        }

        return std::make_pair(neighborX, neighborY);
    }

    //
    // Given a subgrid, the full state dimensions and the offset vector of 
    // a neighbor (neighborDelta), return the starting coordinates and dimensions
    // of the neighbor which would exist at subgridposition.xy + neighbordelta.xy.
    //
    // This accounts for wrapping in the full state; note that subgrids may self-wrap
    // if they have no neighbors along a particular axis.
    //
    void 
    MaybeCreateNewNeighbors(
        Utility::AlignedMemoryPool<64>& memoryPool,
        const GameOfLife::SubGridPtr& spSubgrid,
        const GameOfLife::SparseGrid& sparseGrid,
        GameOfLife::SubGridGraph& gridGraph,
        std::vector<GameOfLife::SubGridPtr>& subgridPtrsOut
        )
    {
        using namespace GameOfLife;
        for (int i = 0; i < GameOfLife::AdjacencyIndex::MAX; ++i)
        {
            const auto Adjacency = static_cast<AdjacencyIndex>(i);
            const auto NeighborOffset = SubGridGraph::GetNeighborPositionFromIndex(Adjacency);
            if (spSubgrid->IsNextGenerationNeighbor(Adjacency))
            {
                const SubGrid::CoordinateType NeighborCoords = GetNeighborCoordinates(*spSubgrid, sparseGrid, NeighborOffset);
                
                SubGridPtr spNeighbor;
                if (gridGraph.QuerySubgrid(NeighborCoords, spNeighbor))
                {
                    //
                    // In this instance, subgrid neighbors a preexisting subgrid so there's no need to
                    // create a new one. Worth noting that a subgrid may neighbor itself.
                    //
                }
                else
                {
                    //
                    // It's possible that more than one subgrid will demand the same new neighbor.
                    // In that case, we may have already created the neighbor and it lives in subgridPtrsOut.
                    // Check for that here prior to deciding to create a new subgrid.
                    //
                    auto it =
                        std::find_if(subgridPtrsOut.begin(), subgridPtrsOut.end(), [&NeighborCoords](const SubGridPtr& spSubgrid)
                        {
                            return spSubgrid->GetCoordinates() == NeighborCoords;
                        }); 

                    if (it == subgridPtrsOut.end())
                    {
                        subgridPtrsOut.emplace_back(
                            std::make_shared<SubGrid>(
                                memoryPool, sparseGrid, gridGraph,
                                NeighborCoords.first, NeighborCoords.second,
                                spSubgrid->GetGeneration()
                            ));
                        spNeighbor = subgridPtrsOut.back();
                    }
                    else
                    {
                        spNeighbor = *it;
                    }
                }

                spNeighbor->CopyBorder(*spSubgrid, GetReflectedAdjacencyIndex(Adjacency));
            }
        }
    }

    //
    // If a subgrid is worth retiring (has no more live cells or border cells),
    // tack it onto the end of subgridPtrsOut for later processing.
    //
    void MaybeRetireSubgrid(
        uint32_t numCells,
        GameOfLife::SubGridPtr spSubgrid,
        std::vector<GameOfLife::SubGridPtr>& subgridPtrsOut
        )
    {
        if (!numCells && !spSubgrid->HasBorderCells())
        {
            subgridPtrsOut.push_back(spSubgrid);
        }
    }
}

namespace GameOfLife
{
    SparseGrid::SparseGrid(
        const std::vector<Cell>& initialCells,
        Utility::AlignedMemoryPool<64>& memoryPool
    ) : m_alignedPool(memoryPool), m_generationCount(0)
    {
        assert(!initialCells.empty());

        int64_t xMin = std::numeric_limits<int64_t>::max();
        int64_t xMax = std::numeric_limits<int64_t>::min();
        int64_t yMin = std::numeric_limits<int64_t>::max();
        int64_t yMax = std::numeric_limits<int64_t>::min();

        // 
        // Determine the dimensions of the world space based on the incoming cells,
        // though be sure to align min/max values with the subgrid corners.
        //

        for (const Cell& cell : initialCells)
        {
            if (cell.X < xMin) { xMin = cell.X; }
            if (cell.X > xMax) { xMax = cell.X; }
            if (cell.Y < yMin) { yMin = cell.Y; }
            if (cell.Y > yMax) { yMax = cell.Y; }
        }

        //
        // Snap state to subgrid boundaries.
        //
        xMin = SubGrid::SnapCoordinateToSubgridCorner(xMin, SubGrid::SUBGRID_WIDTH);
        yMin = SubGrid::SnapCoordinateToSubgridCorner(yMin, SubGrid::SUBGRID_HEIGHT);

        //
        // Do the same for max values -- though max values are on the lower-right corner.
        //
        xMax = SubGrid::SnapCoordinateToSubgridCorner(xMax, SubGrid::SUBGRID_WIDTH);
        xMax += SubGrid::SUBGRID_WIDTH;
        yMax = SubGrid::SnapCoordinateToSubgridCorner(yMax, SubGrid::SUBGRID_HEIGHT);
        yMax += SubGrid::SUBGRID_HEIGHT;

        m_xMin  = xMin;
        m_width = std::max(xMax - xMin, SubGrid::SUBGRID_WIDTH);
        assert(!(m_width % SubGrid::SUBGRID_WIDTH));

        m_yMin   = yMin;
        m_height = std::max(yMax - yMin, SubGrid::SUBGRID_HEIGHT);
        assert(!(m_height % SubGrid::SUBGRID_HEIGHT));

        for (const Cell& cell : initialCells)
        {
            //
            // Snap upper-left coordinates of subgrids to boundaries on SUBGRID_WIDTH
            // and SUBGRID_HEIGHT for x,y respectively.
            //
            int64_t subgridMinX = SubGrid::SnapCoordinateToSubgridCorner(cell.X, SubGrid::SUBGRID_WIDTH);
            int64_t subgridMinY = SubGrid::SnapCoordinateToSubgridCorner(cell.Y, SubGrid::SUBGRID_HEIGHT);

            SubGridPtr spSubgrid;
            if (!m_gridGraph.QuerySubgrid(std::make_pair(subgridMinX, subgridMinY), /* out */spSubgrid))
            {
                auto spSubgrid = std::make_shared<SubGrid>(
                        m_alignedPool, *this, m_gridGraph,
                        subgridMinX, subgridMinY
                    );
                spSubgrid->RaiseCell(cell.X, cell.Y);
                if (!m_subgridStorage.Add(spSubgrid))
                {
                    assert(false);
                    throw std::exception("Unrecoverable: Could not add new subgrid to storage!");
                }

                if (!m_gridGraph.AddSubgrid(spSubgrid))
                {
                    assert(false);
                    throw std::exception("Unrecoverable: Could not add new subgrid to graph!");
                }
            }
            else
            {
                spSubgrid->RaiseCell(cell.X, cell.Y);
            }
        }

        if (m_subgridStorage.GetSize() == 0)
        {
            //
            // No subgrids makes for a pretty boring simulation. The program entry point
            // should have caught this, so just throw.
            //

            throw std::exception("Unrecoverable: No subgrids created.");
        }

        //
        // Initialize subgrid neighbor relationships in the graph and create new
        // neighbors if necessary.
        //

        std::vector<SubGridPtr> subgridsToAdd;
        for (auto it = m_subgridStorage.begin(); it != m_subgridStorage.end(); ++it)
        {
            SubGridPtr spSubGrid = it->second;
            MaybeCreateNewNeighbors(m_alignedPool, spSubGrid, *this, m_gridGraph, subgridsToAdd);
        }

        const size_t NumAdded = subgridsToAdd.size();
        if (NumAdded)
        {
            //
            // We've got a nasty bug somewhere if there are any duplicates, so fail
            // hard if that's the case.
            //
            if (!m_subgridStorage.Add(subgridsToAdd))
            {
                assert(false);
                throw std::exception("Uncrecoverable: Could not add new subgrids to storage!");
            }

            if (!m_gridGraph.AddSubgrids(subgridsToAdd))
            {
                assert(false);
                throw std::exception("Uncrecoverable: Could not add new subgrids to graph!");
            }
        }

        PopulateAdjacencyInfo(m_subgridStorage.begin(), m_subgridStorage.end());
    }

    bool SparseGrid::AdvanceGeneration()
    {
        std::vector<SubGridPtr> subgridsToAdd;
        std::vector<SubGridPtr> subgridsToRemove;
        for (auto it = m_subgridStorage.begin(); it != m_subgridStorage.end(); ++it)
        {
            auto spSubgrid = it->second;
            const uint32_t NumCells = spSubgrid->AdvanceGeneration();

            MaybeCreateNewNeighbors(m_alignedPool, spSubgrid, *this, m_gridGraph, subgridsToAdd);
            MaybeRetireSubgrid(NumCells, spSubgrid, subgridsToRemove);
        }

        const size_t NumRemoved = subgridsToRemove.size();
        for (size_t i = 0; i < NumRemoved; i++)
        {
            SubGridPtr spSubgrid = subgridsToRemove[i];
            m_gridGraph.RemoveSubgrid(spSubgrid);
            m_subgridStorage.Remove(spSubgrid);
        }

        const size_t NumAdded = subgridsToAdd.size();
        if (NumAdded)
        {
            if (!m_subgridStorage.Add(subgridsToAdd))
            {
                assert(false);
                throw std::exception("Unrecoverable: Could not add new subgrid to storage!");
            }

            if (!m_gridGraph.AddSubgrids(subgridsToAdd))
            {
                assert(false);
                throw std::exception("Unrecoverable: Could not add new subgrid to graph!");
            }

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
        const int64_t xMax = m_xMin + m_width;
        const int64_t yMax = m_yMin + m_height;

        for (auto it = begin; it != end; ++it)
        {
            const SubGridPtr spSubgrid = it->second;
            const auto SubgridCoordinates = std::make_pair(spSubgrid->XMin(), spSubgrid->YMin());
            for (int i = 0; i < AdjacencyIndex::MAX; i++)
            {
                const AdjacencyIndex Adjacency = static_cast<AdjacencyIndex>(i);
                const auto Delta = SubGridGraph::GetNeighborPositionFromIndex(Adjacency);
                const auto NeighborCoordinates = GetNeighborCoordinates(*spSubgrid, *this, Delta);

                SubGridPtr spNeighbor;
                if (m_gridGraph.QuerySubgrid(NeighborCoordinates, spNeighbor))
                {
                    m_gridGraph.AddEdge(spSubgrid, spNeighbor, Adjacency);
                }
            }
        }
    }

    std::ostream& operator<<(std::ostream& out, const SparseGrid& grid)
    {
        out << "(" << grid.XMin() << "," << grid.YMin() << "," << grid.Width() << "," << grid.Height() << ")\n"
            << grid.m_generationCount << "\n"
            << grid.m_subgridStorage.GetSize() << "\n";

        for (auto it = grid.begin(); it != grid.end(); ++it)
        {
            const SubGridPtr& spSubgrid = it->second;
            out << "(" << spSubgrid->XMin() << "," << spSubgrid->YMin() << "," << spSubgrid->Width() << "," << spSubgrid->Height() << ")\n";

            const int64_t YMax = spSubgrid->YMin() + spSubgrid->Height();
            const int64_t XMax = spSubgrid->XMin() + spSubgrid->Width();
            for (int64_t y = spSubgrid->YMin(); y < YMax; y++)
            {
                for (int64_t x = spSubgrid->XMin(); x < XMax; x++)
                {
                    if (spSubgrid->GetCellState(x, y))
                    {
                        out << "1";
                    }
                    else
                    {
                        out << "0";
                    }

                    if (x < XMax - 1)
                    {
                        out << ",";
                    }
                }
                out << "\n";
            }
        }
        out.flush();

        return out;
    }
}
