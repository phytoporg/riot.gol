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

        //
        // Helper function which translates a direction vector e.g. ((1,1), (-1,1), etc) 
        // to its cooresponding AdjacencyIndex. Just simplifies a few loops.
        //
        static AdjacencyIndex GetIndexFromNeighborPosition(SubGrid::CoordinateType coord);

        //
        // Inverse of the above-- from an AdjacencyInded value, produce a direction vector
        // as a CoordinateType.
        //
        static SubGrid::CoordinateType GetNeighborPositionFromIndex(AdjacencyIndex coord);

        //
        // Given subgrid pointer spSubgrid, return an array of raw pointers whose
        // indices correspond to neighbors in positions matching AdjacencyIndex values.
        // For instance, index 0 of pspNeighbors upon return points to the TOP_LEFT
        // neighbor, or is null if no such neighbor exists.
        //
        // We store and return raw pointer arrays as not to create circular smart
        // pointer dependencies in the graph.
        //
        // Returns false if no matching subgrid can be found in the graph.
        //
        bool GetNeighborArray(SubGrid const* pSubgrid, /* Out */ SubGrid**& ppNeighbors) const;
        bool GetNeighborArray(const SubGridPtr spSubgrid, /* Out */ SubGrid**& ppNeighbors) const;

        bool AddSubgrid(SubGridPtr spSubgrid);
        bool AddSubgrids(const std::vector<SubGridPtr>& subgridPtrs);

        //
        // Removes a subgrid from the graph but also clears appropriate subgrid borders.
        //
        bool RemoveSubgrid(const SubGridPtr& spSubgrid);

        //
        // Queries for a subgrid in the graph at the location specified by coord, returns
        // false if no such subgrid exists.
        //
        // In some instances, we're not actually interested in retrieving the subgrid
        // at the queried coordinates, hence the overload.
        //
        bool QuerySubgrid(const SubGrid::CoordinateType& coord, SubGridPtr& spSubGrid) const;
        bool QuerySubgrid(const SubGrid::CoordinateType& coord) const;

        //
        // Note that here we do not initialize subgrid borders, since it's
        // unclear which subgrid is new and the graph is bidirectional.
        //
        bool AddEdge(
            SubGridPtr spSubgrid1,
            SubGridPtr spSubgrid2,
            AdjacencyIndex adjacency
            );

    private:
        SubGridGraph(const SubGridGraph& other) = delete;
        SubGridGraph& operator=(const SubGridGraph& other) = delete;

        //
        // Pimpl to avoid polluting other header files-- the current
        // implementation relies on tuple hash include which few other 
        // modules require.
        //
        struct Pimpl;
        std::unique_ptr<Pimpl> m_spPimpl;
    };
}
