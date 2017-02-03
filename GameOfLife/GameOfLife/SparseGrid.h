#pragma once

// 
// The full game state. Just a 2D array for now.
//

#include "SubGrid.h"
#include "SubgridStorage.h"
#include "Cell.h"
#include "RectangularGrid.h"
#include "SubgridGraph.h"

#include <Utility/AlignedMemoryPool.h>

#include <vector>

namespace GameOfLife
{
    class SparseGrid : public RectangularGrid
    {
    public:
        SparseGrid(const std::vector<Cell>& initialState);
        ~SparseGrid();

        bool AdvanceGeneration();

        uint32_t GetGeneration() const { return m_generationCount; }

        SubgridStorage::iterator begin() { return m_subgridStorage.begin(); }
        SubgridStorage::iterator end()   { return m_subgridStorage.end(); }
        SubgridStorage::const_iterator begin() const { return m_subgridStorage.begin(); }
        SubgridStorage::const_iterator end()   const { return m_subgridStorage.end(); }

    private:
        SparseGrid() = delete;
        SparseGrid(const SparseGrid& other) = delete;
        SparseGrid& operator=(const SparseGrid& other) = delete;

        void 
        PopulateAdjacencyInfo(
            SubgridStorage::const_iterator begin,
            SubgridStorage::const_iterator end
            );

        //
        // Subgrid storage. TODO: Don't use a vector. Will have to incur a linear
        // lookup and "shuffle" cost for deletion. Fine for now, but not great at
        // scale.
        //
        SubgridStorage m_subgridStorage;

        //
        // Subgrid adjacency info
        //
        SubGridGraph m_gridGraph;

        Utility::AlignedMemoryPool<64> m_alignedPool;
        uint32_t m_generationCount;
    };
}