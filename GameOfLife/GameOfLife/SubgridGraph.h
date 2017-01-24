#pragma once

// 
// Undirected graph of subgrids for looking up and managing subgrid neighbor
// relationships.
//

#include "SubGrid.h"

#include <unordered_map>
#include <utility>
#include <functional>
#include <memory>

namespace GameOfLife
{
    enum AdjacencyIndex
    {
        TOP_LEFT = 0,
        TOP,
        TOP_RIGHT,
        LEFT,
        RIGHT,
        BOTTOM_LEFT,
        BOTTOM,
        BOTTOM_RIGHT,
        MAX
    };

    class SubGridGraph
    {
    public:
        SubGridGraph();
        ~SubGridGraph();

        static AdjacencyIndex GetIndexFromNeighborPosition(SubGrid::CoordinateType coord);

        bool GetNeighborArray(const SubGrid& subgrid, SubGrid**& ppNeighbors) const;

        bool AddVertex(const SubGrid& subgrid);
        bool RemoveVertex(const SubGrid& subgrid);
        bool QueryVertex(
            const SubGrid::CoordinateType& coord,
            SubGrid** ppSubGrid
            ) const;

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
