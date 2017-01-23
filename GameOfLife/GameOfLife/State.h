#pragma once

// 
// The full game state. Just a 2D array for now.
//

#include "InitialState.h"
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
        static const int64_t SUBGRID_WIDTH  = 20;
        static const int64_t SUBGRID_HEIGHT = 20;

        State(const InitialState& initialState);

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
