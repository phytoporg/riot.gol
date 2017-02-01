#include "AdjacencyIndex.h"

#include <cassert>

namespace GameOfLife
{
    AdjacencyIndex GetReflectedAdjacencyIndex(AdjacencyIndex index)
    {
        using GameOfLife::AdjacencyIndex;

        //
        // Bounds check
        //
        assert(index >= 0 && index < AdjacencyIndex::MAX);
        return static_cast<AdjacencyIndex>(AdjacencyIndex::MAX - 1 - index);
    }
}
