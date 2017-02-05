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
        // If any of the subgrids attempting to be added are already in
        // storage, Add() returns false and no new subgrids are added.
        //
        bool Add(const SubGridPtr spSubgrid);
        bool Add(const std::vector<SubGridPtr>& subgrids);

        //
        // Returns true if the provided subgrid already exists in 
        // storage.
        //
        bool Query(const SubGrid::CoordinateType& coordinate, /* out */SubGridPtr& spSubgridOut);

        ~SubgridStorage();

        // 
        // Returns false if attempting to remove subgrid which does
        // not exist in storage.
        //
        bool Remove(const SubGridPtr& spSubgrid);

        size_t GetSize() const { return m_subgridMap.size(); }

    private:
        std::unordered_map<SubGrid::CoordinateType, SubGridPtr> m_subgridMap;

    public:
        using iterator       = decltype(m_subgridMap)::iterator;
        using const_iterator = decltype(m_subgridMap)::const_iterator;

        iterator begin() { return m_subgridMap.begin(); }
        iterator end()   { return m_subgridMap.end();   }
        const_iterator begin() const { return m_subgridMap.begin(); }
        const_iterator end()   const { return m_subgridMap.end();   }
    };
}

