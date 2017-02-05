#include "SubgridStorage.h"
#include "SubgridGraph.h"

#include <unordered_map>

namespace GameOfLife
{
    SubgridStorage::~SubgridStorage() = default;

    bool SubgridStorage::Add(const SubGridPtr spSubgrid)
    {
        const SubGrid::CoordinateType& Coordinates = spSubgrid->GetCoordinates();
        if (m_subgridMap.find(Coordinates) != m_subgridMap.end())
        {
            return false;
        }

        m_subgridMap[Coordinates] = spSubgrid;
        return true;
    }

    bool SubgridStorage::Add(const std::vector<SubGridPtr>& subgridPtrs)
    {
        //
        // Don't add anything if there's a single duplicate.
        //
        for (const auto& spSubgrid : subgridPtrs)
        {
            if (m_subgridMap.find(spSubgrid->GetCoordinates()) != m_subgridMap.end())
            {
                return false;
            }
        }

        //
        // No duplicates found. Go ahead and add all subgrids in the vector.
        //
        for (const auto& spSubgrid : subgridPtrs)
        {
            const SubGrid::CoordinateType& Coordinates = spSubgrid->GetCoordinates();
            m_subgridMap[Coordinates] = spSubgrid;
        }

        return true;
    }

    bool SubgridStorage::Query(const SubGrid::CoordinateType& coord, SubGridPtr& spSubgridOut)
    {
        auto it = m_subgridMap.find(coord);
        if (it != m_subgridMap.end())
        {
            spSubgridOut = it->second;
            return true;
        }

        return false;
    }

    bool SubgridStorage::Remove(const SubGridPtr& spSubgrid)
    {
        //
        // Return false if we didn't actually erase anything because this
        // subgrid was not found.
        //
        return m_subgridMap.erase(spSubgrid->GetCoordinates()) > 0;
    }
}
