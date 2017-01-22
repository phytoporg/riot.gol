#include "SubGridGraph.h"

#include <Utility/Hash.h>

#include <cassert>

namespace
{
    GameOfLife::SubGrid::CoordinateType GetSubridCoordinates(
        const GameOfLife::SubGrid& subgrid
        )
    {
        return std::make_pair(subgrid.XMin(), subgrid.YMin());
    }

    GameOfLife::AdjacencyIndex GetIndexFromNeighborPosition(
        const GameOfLife::SubGrid::CoordinateType& coord
        )
    {
        using GameOfLife::AdjacencyIndex;
        static const AdjacencyIndex LUT[3][3] =
        {
            { AdjacencyIndex::TOP_LEFT,    AdjacencyIndex::TOP,    AdjacencyIndex::TOP_RIGHT      },
            { AdjacencyIndex::LEFT,        AdjacencyIndex::MAX,    AdjacencyIndex::RIGHT          },
            { AdjacencyIndex::BOTTOM_LEFT, AdjacencyIndex::BOTTOM, AdjacencyIndex::BOTTOM_RIGHT}
        };

        //
        // (0,0) is not a neighbor
        //
        assert(coord.first || coord.second); 

        //
        // Bounds check
        //
        assert(coord.first < 3 && coord.second < 3);
        assert(coord.first > 0 && coord.second > 0);
        return LUT[coord.first + 1][coord.second + 1];
    }

    GameOfLife::AdjacencyIndex GetReflectedAdjacencyIndex(
        const GameOfLife::AdjacencyIndex& index
        )
    {
        using GameOfLife::AdjacencyIndex;
        static const AdjacencyIndex LUT[AdjacencyIndex::MAX] =
        {
            AdjacencyIndex::BOTTOM_RIGHT,
            AdjacencyIndex::BOTTOM,
            AdjacencyIndex::BOTTOM_LEFT,
            AdjacencyIndex::RIGHT,
            AdjacencyIndex::LEFT,
            AdjacencyIndex::TOP_RIGHT,
            AdjacencyIndex::TOP,
            AdjacencyIndex::TOP_LEFT
        };

        //
        // Bounds check
        //
        assert(index > 0 && index < AdjacencyIndex::MAX);
        return LUT[index];
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

    bool SubGridGraph::AddVertex(const SubGrid& subgrid)
    {
        const auto Coords = GetSubridCoordinates(subgrid);
        
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
        const auto Coords = GetSubridCoordinates(subgrid);

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
                
                const auto& NeighborCoords = GetSubridCoordinates(*pNeighbor);
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

    bool SubGridGraph::AddEdge(const SubGrid& subgrid1, const SubGrid& subgrid2)
    {
        const auto Coordinates1 = GetSubridCoordinates(subgrid1);
        const auto Coordinates2 = GetSubridCoordinates(subgrid2);

        auto it1 = m_spPimpl->VertexLookup.find(Coordinates1);
        if (it1 == m_spPimpl->VertexLookup.end()) { return false; }

        auto it2 = m_spPimpl->VertexLookup.find(Coordinates2);
        if (it2 == m_spPimpl->VertexLookup.end()) { return false; }

        const auto OneToTwo = std::make_pair(
            Coordinates2.first - Coordinates1.first,
            Coordinates2.second - Coordinates1.second
            );
        const AdjacencyIndex OneToTwoIndex = GetIndexFromNeighborPosition(OneToTwo);
        const AdjacencyIndex TwoToOneIndex = GetReflectedAdjacencyIndex(OneToTwoIndex);

        it1->second.Neighbors[OneToTwoIndex] = it2->second.pSubGrid;
        it2->second.Neighbors[TwoToOneIndex] = it1->second.pSubGrid;

        return true;
    }

    bool SubGridGraph::RemoveEdge(const SubGrid& subgrid1, const SubGrid& subgrid2)
    {
        const auto Coordinates1 = GetSubridCoordinates(subgrid1);
        const auto Coordinates2 = GetSubridCoordinates(subgrid2);

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
        std::function<void(SubGrid& subgrid, SubGridGraph& graph)> f
        )
    {
        for (auto p : m_spPimpl->VertexLookup)
        {
            f(*p.second.pSubGrid, *this);
        }
    }
}
