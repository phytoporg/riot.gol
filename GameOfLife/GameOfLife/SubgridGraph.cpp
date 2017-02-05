#include "SubGridGraph.h"
#include "SparseGrid.h"

#include "CoordinateTypeHash.h"

#include <cassert>

namespace
{
    //
    // Gets the adjacency index representing the "opposite" direction
    // of a given neighbor. top -> bottom, left -> right, etc.
    //
    GameOfLife::AdjacencyIndex GetReflectedAdjacencyIndex(
        const GameOfLife::AdjacencyIndex& index
        )
    {
        using GameOfLife::AdjacencyIndex;

        //
        // Bounds check
        //
        assert(index >= 0 && index < AdjacencyIndex::MAX);
        return static_cast<AdjacencyIndex>(AdjacencyIndex::MAX - 1 - index);
    }

    struct LookupValue
    {
        GameOfLife::SubGridPtr spSubGrid;
        GameOfLife::SubGrid* ppNeighbors[8];
    };
}

namespace GameOfLife
{
    struct SubGridGraph::Pimpl
    {
        std::unordered_map<SubGrid::CoordinateType, LookupValue> SubgridLookup;
    };

    SubGridGraph::SubGridGraph()
        : m_spPimpl(new Pimpl)
    {}
    
    //
    // For Pimpl
    //
    SubGridGraph::~SubGridGraph() = default;

    bool SubGridGraph::GetNeighborArray(
        SubGrid const* pSubgrid,
        /* Out */ SubGrid**& ppNeighbors
    ) const
    {
        const SubGrid::CoordinateType& Coordinates = pSubgrid->GetCoordinates();
        auto it = m_spPimpl->SubgridLookup.find(Coordinates);
        if (it == m_spPimpl->SubgridLookup.end())
        {
            return false;
        }

        ppNeighbors = it->second.ppNeighbors;
        return true;
    }

    bool SubGridGraph::GetNeighborArray(
        const SubGridPtr spSubgrid,
        /* out */ SubGrid**& ppNeighbors
        ) const
    {
        return GetNeighborArray(spSubgrid.get(), ppNeighbors);
    }

    AdjacencyIndex SubGridGraph::GetIndexFromNeighborPosition(
        SubGrid::CoordinateType coord
        )
    {
        static const AdjacencyIndex LUT[3][3] =
        {
            { AdjacencyIndex::TOP_LEFT,    AdjacencyIndex::TOP,    AdjacencyIndex::TOP_RIGHT    },
            { AdjacencyIndex::LEFT,        AdjacencyIndex::MAX,    AdjacencyIndex::RIGHT        },
            { AdjacencyIndex::BOTTOM_LEFT, AdjacencyIndex::BOTTOM, AdjacencyIndex::BOTTOM_RIGHT }
        };

        //
        // (0,0) is not a neighbor
        //
        assert(coord.first || coord.second); 

        //
        // Bounds check
        //
        assert(coord.first <= 1 && coord.second <= 1);
        assert(coord.first >= -1 && coord.second >= -1);
        return LUT[coord.second + 1][coord.first + 1];
    }

    SubGrid::CoordinateType SubGridGraph::GetNeighborPositionFromIndex(AdjacencyIndex index)
    {
        static const SubGrid::CoordinateType LUT[] =
        {
            std::make_pair<int64_t, int64_t>(-1, -1), // TOP_LEFT
            std::make_pair<int64_t, int64_t>( 0, -1), // TOP
            std::make_pair<int64_t, int64_t>( 1, -1), // TOP_RIGHT
            std::make_pair<int64_t, int64_t>(-1,  0), // LEFT
            std::make_pair<int64_t, int64_t>( 1,  0), // RIGHT
            std::make_pair<int64_t, int64_t>(-1,  1), // BOTTOM_LEFT
            std::make_pair<int64_t, int64_t>( 0,  1), // BOTTOM
            std::make_pair<int64_t, int64_t>( 1,  1)  // BOTTOM_RIGHT
        };

        assert(index >= 0 && index < AdjacencyIndex::MAX);
        return LUT[index];
    }

    bool SubGridGraph::AddSubgrid(SubGridPtr spSubgrid)
    {
        const auto& Coordinates = spSubgrid->GetCoordinates();
        
        auto it = m_spPimpl->SubgridLookup.find(Coordinates);
        if (it != m_spPimpl->SubgridLookup.end()) { return false; }

        m_spPimpl->SubgridLookup[Coordinates] = { spSubgrid, {} };

        return true;
    }

    bool SubGridGraph::AddSubgrids(const std::vector<SubGridPtr>& subgridPtrs)
    {
        //
        // First pass checks for any duplicates. This is all-or-nothing.
        //
        for (auto& spSubgrid : subgridPtrs)
        {
            const auto& Coordinates = spSubgrid->GetCoordinates();
            auto it = m_spPimpl->SubgridLookup.find(Coordinates);
            if (it != m_spPimpl->SubgridLookup.end()) { return false; }
        }
        
        for (auto& spSubgrid : subgridPtrs)
        {
            const auto& Coordinates = spSubgrid->GetCoordinates();
            m_spPimpl->SubgridLookup[Coordinates] = { spSubgrid, {} };
        }

        return true;
    }

    bool SubGridGraph::RemoveSubgrid(const SubGridPtr& spSubgrid)
    {
        const auto Coordinates = spSubgrid->GetCoordinates();

        auto it = m_spPimpl->SubgridLookup.find(Coordinates);
        if (it == m_spPimpl->SubgridLookup.end()) { return false; }

        //
        // When we remove a vertex we've got to remove its edges as well.
        //
        for (int i = 0; i < AdjacencyIndex::MAX; ++i)
        {
            SubGrid*& pNeighbor = it->second.ppNeighbors[i];
            if (pNeighbor)
            {
                const AdjacencyIndex ReflectedIndex =
                    GetReflectedAdjacencyIndex(static_cast<AdjacencyIndex>(i));

                pNeighbor->ClearBorder(ReflectedIndex);
                spSubgrid->ClearBorder(static_cast<AdjacencyIndex>(i));
                
                const SubGrid::CoordinateType& NeighborCoords = pNeighbor->GetCoordinates();
                SubGrid*& pNeighborNeighbor = m_spPimpl->SubgridLookup[NeighborCoords].ppNeighbors[ReflectedIndex];

                //
                // Asymmetry in the graph. Shouldn't happen.
                //
                assert(pNeighborNeighbor == it->second.spSubGrid.get());

                //
                // Clear respective entries for either subgrid.
                //
                pNeighborNeighbor = nullptr;
                pNeighbor = nullptr;
            }
        }

        m_spPimpl->SubgridLookup.erase(Coordinates);

        return true;
    }

    bool SubGridGraph::QuerySubgrid(
        const SubGrid::CoordinateType& coord,
        SubGridPtr& spSubGrid
        ) const 
    {
        auto it = m_spPimpl->SubgridLookup.find(coord); 
        if (it == m_spPimpl->SubgridLookup.end())
        {
            return false;
        }

        spSubGrid = it->second.spSubGrid;

        return true;
    }

    bool SubGridGraph::QuerySubgrid(const SubGrid::CoordinateType& coord) const 
    {
        auto it = m_spPimpl->SubgridLookup.find(coord); 
        return it != m_spPimpl->SubgridLookup.end();
    }

    bool SubGridGraph::AddEdge(
        SubGridPtr spSubgrid1,
        SubGridPtr spSubgrid2,
        AdjacencyIndex adjacency
        )
    {
        auto it1 = m_spPimpl->SubgridLookup.find(spSubgrid1->GetCoordinates());
        if (it1 == m_spPimpl->SubgridLookup.end()) { return false; }

        auto it2 = m_spPimpl->SubgridLookup.find(spSubgrid2->GetCoordinates());
        if (it2 == m_spPimpl->SubgridLookup.end()) { return false; }

        const AdjacencyIndex OneToTwoIndex = adjacency;
        const AdjacencyIndex TwoToOneIndex = GetReflectedAdjacencyIndex(adjacency);

        it1->second.ppNeighbors[OneToTwoIndex] = spSubgrid2.get();
        it2->second.ppNeighbors[TwoToOneIndex] = spSubgrid1.get();

        return true;
    }
}
