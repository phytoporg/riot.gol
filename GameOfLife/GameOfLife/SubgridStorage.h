#pragma once

#include <vector>
#include <unordered_map>

#include "Subgrid.h"
#include "CoordinateTypeHash.h"

namespace GameOfLife
{
    class SubGridGraph;

    //
    // Tracks subgrids and facilitates managing their life cycles.
    //
    class SubgridStorage
    {
    public:
        //
        // Returns false if attempting to insert duplicates.
        //
        bool Add(const SubGrid& subgrid, SubGridGraph& graph);
        bool Add(const std::vector<SubGrid>& subgrids, SubGridGraph& graph);

        ~SubgridStorage();

        // 
        // Returns false if attempting to remove subgrid which does
        // not exist in storage.
        //
        bool Remove(const SubGrid& subgrid);

        size_t GetSize() const { return m_subgridMap.size(); }

    private:
        std::unordered_map<SubGrid::CoordinateType, SubGrid> m_subgridMap;

    public:
        using iterator       = decltype(m_subgridMap)::iterator;
        using const_iterator = decltype(m_subgridMap)::const_iterator;

        iterator begin() { return m_subgridMap.begin(); }
        iterator end()   { return m_subgridMap.end();   }
        const_iterator begin() const { return m_subgridMap.begin(); }
        const_iterator end()   const { return m_subgridMap.end();   }
    };
}

