#include "SubGridGraph.h"
#include "State.h"

#include <Utility/Hash.h>

#include <cassert>

namespace
{
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
        GameOfLife::SubGrid* pSubGrid;
        GameOfLife::SubGrid* Neighbors[8];
    };
}

namespace std
{
    template<>
    struct hash<GameOfLife::SubGrid::CoordinateType>
    {
        std::size_t operator()(const GameOfLife::SubGrid::CoordinateType& coord) const
        {
            return Utility::hash_value(coord);
        }
    };
}

namespace GameOfLife
{
    struct SubGridGraph::Pimpl
    {
        std::unordered_map<SubGrid::CoordinateType, LookupValue> VertexLookup;
    };

    SubGridGraph::SubGridGraph()
        : m_spPimpl(new Pimpl)
    {}
    
    //
    // For Pimpl
    //
    SubGridGraph::~SubGridGraph() = default;

    bool SubGridGraph::GetNeighborArray(
        const SubGrid& subgrid,
        /* out */SubGrid**& ppNeighbors
        ) const
    {
        assert(ppNeighbors);

        auto it = m_spPimpl->VertexLookup.find(subgrid.GetCoordinates());
        if (it == m_spPimpl->VertexLookup.end())
        {
            return false;
        }

        ppNeighbors = it->second.Neighbors;
        return true;
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

        assert(index > 0 && index < AdjacencyIndex::MAX);
        return LUT[index];
    }

    bool SubGridGraph::AddVertex(const SubGrid& subgrid)
    {
        const auto Coords = subgrid.GetCoordinates();
        
        auto it = m_spPimpl->VertexLookup.find(Coords);
        if (it != m_spPimpl->VertexLookup.end()) { return false; }

        m_spPimpl->VertexLookup[Coords] = { const_cast<SubGrid*>(&subgrid), {} };

        return true;
    }

    bool SubGridGraph::RemoveVertex(const SubGrid& subgrid)
    {
        const auto Coords = subgrid.GetCoordinates();

        auto it = m_spPimpl->VertexLookup.find(Coords);
        if (it == m_spPimpl->VertexLookup.end()) { return false; }

        //
        // When we remove a vertex we've got to remove its edges as well.
        //
        for (int i = 0; i < AdjacencyIndex::MAX; ++i)
        {
            auto& pNeighbor = it->second.Neighbors[i];
            if (pNeighbor)
            {
                const AdjacencyIndex Reflectedindex =
                    GetReflectedAdjacencyIndex(static_cast<AdjacencyIndex>(i));
                
                const auto& NeighborCoords = pNeighbor->GetCoordinates();
                auto& pNeighborNeighbor = m_spPimpl->VertexLookup[NeighborCoords].Neighbors[Reflectedindex];

                //
                // Asymmetry in the graph. Shouldn't happen.
                //
                assert(pNeighborNeighbor == it->second.pSubGrid);

                pNeighborNeighbor = nullptr;
                pNeighbor = nullptr;
            }
        }

        m_spPimpl->VertexLookup.erase(Coords);

        return true;
    }

    bool SubGridGraph::QueryVertex(
        const SubGrid::CoordinateType& coord,
        SubGrid** ppSubGrid = nullptr
        ) const 
    {
        auto it = m_spPimpl->VertexLookup.find(coord); 
        if (it == m_spPimpl->VertexLookup.end())
        {
            return false;
        }

        if (ppSubGrid)
        {
            *ppSubGrid = it->second.pSubGrid;
        }

        return true;
    }

    bool SubGridGraph::AddEdge(
        const SubGrid& subgrid1,
        const SubGrid& subgrid2,
        AdjacencyIndex adjacency
        )
    {
        auto it1 = m_spPimpl->VertexLookup.find(subgrid1.GetCoordinates());
        if (it1 == m_spPimpl->VertexLookup.end()) { return false; }

        auto it2 = m_spPimpl->VertexLookup.find(subgrid2.GetCoordinates());
        if (it2 == m_spPimpl->VertexLookup.end()) { return false; }

        const AdjacencyIndex OneToTwoIndex = adjacency;
        const AdjacencyIndex TwoToOneIndex = GetReflectedAdjacencyIndex(adjacency);

        it1->second.Neighbors[OneToTwoIndex] = const_cast<SubGrid*>(&subgrid2);
        it2->second.Neighbors[TwoToOneIndex] = const_cast<SubGrid*>(&subgrid1);

        return true;
    }
}
