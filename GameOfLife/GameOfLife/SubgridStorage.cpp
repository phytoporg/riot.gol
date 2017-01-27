#include "SubgridStorage.h"
#include "SubgridGraph.h"

#include <unordered_map>

namespace GameOfLife
{
    SubgridStorage::~SubgridStorage() = default;

    bool SubgridStorage::Add(const SubGrid& subgrid, SubGridGraph& graph)
    {
        if (m_subgridMap.find(subgrid.GetCoordinates()) != m_subgridMap.end())
        {
            return false;
        }

        m_subgridMap[subgrid.GetCoordinates()] = subgrid;
        graph.AddVertex(m_subgridMap[subgrid.GetCoordinates()]);

        return true;
    }

    bool SubgridStorage::Add(const std::vector<SubGrid>& subgrids, SubGridGraph& graph)
    {
        //
        // Don't add anything if there's a single duplicate.
        //
        for (const auto& subgrid : subgrids)
        {
            if (m_subgridMap.find(subgrid.GetCoordinates()) != m_subgridMap.end())
            {
                return false;
            }
        }

        for (const auto& subgrid : subgrids)
        {
            m_subgridMap[subgrid.GetCoordinates()] = subgrid;
            graph.AddVertex(m_subgridMap[subgrid.GetCoordinates()]);
        }

        return true;
    }

    bool SubgridStorage::Remove(const SubGrid& subgrid)
    {
        if (m_subgridMap.find(subgrid.GetCoordinates()) == m_subgridMap.end())
        {
            return false;
        }

        m_subgridMap.erase(subgrid.GetCoordinates());
        return true;
    }
}
