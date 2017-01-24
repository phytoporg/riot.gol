#pragma once

// 
// The full game state. Just a 2D array for now.
//

#include "SubGrid.h"
#include "Cell.h"
#include "RectangularGrid.h"
#include "SubgridGraph.h"

#include <vector>

namespace GameOfLife
{
    class State : public RectangularGrid
    {
    public:
        State(const std::vector<Cell>& initialState);

        bool AdvanceGeneration();
        const std::vector<SubGrid>& GetSubgrids() const; 

    private:
        State() = delete;
        State(const State& other) = delete;
        State& operator=(const State& other) = delete;

        //
        // Subgrid storage. TODO: Don't use a vector. Will have to incur a linear
        // lookup and "shuffle" cost for deletion. Fine for now, but not great at
        // scale.
        //
        std::vector<SubGrid> m_subgrids;

        //
        // Subgrid adjacency info
        //
        SubGridGraph m_gridGraph;
    };
}
