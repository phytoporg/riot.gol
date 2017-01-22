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

        bool AddVertex(const SubGrid& subgrid);
        bool RemoveVertex(const SubGrid& subgrid);

        bool AddEdge(const SubGrid& subgrid1, const SubGrid& subgrid2);
        bool RemoveEdge(const SubGrid& subgrid1, const SubGrid& subgrid2);

        //
        // Executes f(s, *this) for each subgrid s in the container.
        //
        void ProcessVertices(
            std::function<void(SubGrid& subgrid, SubGridGraph& graph)> f
            );

    private:
        SubGridGraph(const SubGridGraph& other) = delete;
        SubGridGraph& operator=(const SubGridGraph& other) = delete;

        bool 
        RemoveEdge(
            const SubGrid::CoordinateType& coord1,
            const SubGrid::CoordinateType& coord2
            );

        struct Pimpl;
        std::unique_ptr<Pimpl> m_spPimpl;
    };
}
