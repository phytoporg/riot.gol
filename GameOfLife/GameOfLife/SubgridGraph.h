#pragma once

// 
// Undirected graph of subgrids for looking up and managing subgrid neighbor
// relationships.
//

#include "SubGrid.h"
#include "AdjacencyIndex.h"

#include <unordered_map>
#include <utility>
#include <functional>
#include <memory>

namespace GameOfLife
{
    class SubGridGraph
    {
    public:
        SubGridGraph();
        ~SubGridGraph();

        static AdjacencyIndex GetIndexFromNeighborPosition(SubGrid::CoordinateType coord);
        static SubGrid::CoordinateType GetNeighborPositionFromIndex(AdjacencyIndex coord);

        bool GetNeighborArray(const SubGrid& subgrid, SubGrid**& ppNeighbors) const;

        bool AddVertex(const SubGrid& subgrid);

        //
        // Also clears appropriate subgrid borders.
        //
        bool RemoveVertex(SubGrid& subgrid);
        bool QueryVertex(
            const SubGrid::CoordinateType& coord,
            SubGrid** ppSubGrid
            ) const;

        //
        // Note that here we do not initialize subgrid borders, since it's
        // unclear which subgrid is new and the graph is bidirectional.
        //
        bool AddEdge(
            const SubGrid& subgrid1,
            const SubGrid& subgrid2,
            AdjacencyIndex adjacency
            );

    private:
        SubGridGraph(const SubGridGraph& other) = delete;
        SubGridGraph& operator=(const SubGridGraph& other) = delete;

        struct Pimpl;
        std::unique_ptr<Pimpl> m_spPimpl;
    };
}
