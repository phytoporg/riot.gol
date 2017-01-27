#pragma once

//
// Include in implementation files which require hash functions
// for the SubGrid::CoordinateType pair types.
//

#include "SubGrid.h"

#include <Utility\Hash.h>

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
