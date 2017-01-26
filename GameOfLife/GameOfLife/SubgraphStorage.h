#pragma once

#include <vector>
#include <unordered_map>

#include "Subgrid.h"

namespace GameOfLife
{
    //
    // Tracks subgrids and facilitates managing their life cycles.
    //
    class SubgraphStorage
    {
    public:
        //
        // Returns false if attempting to insert duplicates.
        //
        bool Add(const SubGrid& subgrid);
        bool Add(const std::vector<SubGrid>& subgrids);

        // 
        // Returns false if attempting to remove subgrid which does
        // not exist in storage.
        //
        bool Remove(const SubGrid& subgrid);

    private:
        std::unordered_map<SubGrid::CoordinateType, SubGrid> m_subgridMap;
    };
}

