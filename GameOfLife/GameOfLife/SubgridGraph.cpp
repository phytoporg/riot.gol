#include "SubGridGraph.h"
#include "State.h"

#include <Utility/Hash.h>

#include <cassert>

namespace
{
    GameOfLife::AdjacencyIndex GetIndexFromNeighborPosition(
        const GameOfLife::SubGrid::CoordinateType& coord
        )
    {
        using GameOfLife::AdjacencyIndex;
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

    bool SubGridGraph::AddVertex(const SubGrid& subgrid)
    {
        const auto Coords = subgrid.GetCoordinates();
        
        auto it = m_spPimpl->VertexLookup.find(Coords);
        if (it != m_spPimpl->VertexLookup.end()) { return false; }

        //
        // This function won't modify subgrid, but no gaurantee that
        // it remains untouched following a call to ProcessVertices().
        //
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

    bool SubGridGraph::AddEdge(const SubGrid& subgrid1, const SubGrid& subgrid2)
    {
        const auto Coordinates1 = subgrid1.GetCoordinates();
        const auto Coordinates2 = subgrid2.GetCoordinates();

        return AddEdge(Coordinates1, Coordinates2);
    }

    bool SubGridGraph::AddEdge(
        const SubGrid::CoordinateType& coord1,
        const SubGrid::CoordinateType& coord2
        )
    {
        auto it1 = m_spPimpl->VertexLookup.find(coord1);
        if (it1 == m_spPimpl->VertexLookup.end()) { return false; }

        auto it2 = m_spPimpl->VertexLookup.find(coord2);
        if (it2 == m_spPimpl->VertexLookup.end()) { return false; }

        const auto OneToTwo = std::make_pair(
            (coord2.first  - coord1.first)  / State::SUBGRID_WIDTH,
            (coord2.second - coord1.second) / State::SUBGRID_HEIGHT
            );
        const AdjacencyIndex OneToTwoIndex = GetIndexFromNeighborPosition(OneToTwo);
        const AdjacencyIndex TwoToOneIndex = GetReflectedAdjacencyIndex(OneToTwoIndex);

        it1->second.Neighbors[OneToTwoIndex] = it2->second.pSubGrid;
        it2->second.Neighbors[TwoToOneIndex] = it1->second.pSubGrid;

        return true;
    }

    bool SubGridGraph::RemoveEdge(const SubGrid& subgrid1, const SubGrid& subgrid2)
    {
        const auto Coordinates1 = subgrid1.GetCoordinates();
        const auto Coordinates2 = subgrid2.GetCoordinates();

        return RemoveEdge(Coordinates1, Coordinates2);
    }

    bool SubGridGraph::RemoveEdge(
        const SubGrid::CoordinateType& coord1,
        const SubGrid::CoordinateType& coord2
        )
    {
        auto it1 = m_spPimpl->VertexLookup.find(coord1);
        if (it1 == m_spPimpl->VertexLookup.end()) { return false; }

        auto it2 = m_spPimpl->VertexLookup.find(coord2);
        if (it2 == m_spPimpl->VertexLookup.end()) { return false; }

        int i = 0;
        for (; i < AdjacencyIndex::MAX; i++)
        {
            auto& pNeighbor = it1->second.Neighbors[i];
            if (pNeighbor == it2->second.pSubGrid)
            {
                pNeighbor = nullptr;

                const AdjacencyIndex Reflectedindex =
                    GetReflectedAdjacencyIndex(static_cast<AdjacencyIndex>(i));

                //
                // We somehow have asymmetries in the graph edges.
                //
                assert(it2->second.Neighbors[Reflectedindex] != nullptr);
                it2->second.Neighbors[Reflectedindex] = nullptr;
                break;
            }
        }

        if (i >= AdjacencyIndex::MAX)
        {
            return false;
        }

        return true;
    }

    void SubGridGraph::ProcessVertices(
        std::function<void(SubGrid& subgrid, SubGridGraph& graph)>& f
        )
    {
        for (auto p : m_spPimpl->VertexLookup)
        {
            f(*p.second.pSubGrid, *this);
        }
    }
}
