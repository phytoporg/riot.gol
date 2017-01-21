#pragma once

// 
// The full game state. Just a 2D array for now.
//

#include "InitialState.h"
#include "SubGrid.h"
#include "Cell.h"
#include "RectangularGrid.h"

#include <vector>

namespace GameOfLife
{
    class State : public RectangularGrid
    {
    public:
        static const uint64_t SUBGRID_WIDTH  = 20;
        static const uint64_t SUBGRID_HEIGHT = 20;

        State(const InitialState& initialState);

        bool AdvanceGeneration();
        const std::vector<SubGrid>& GetSubgrids() const; 

    private:
        State() = delete;
        State(const State& other) = delete;
        State& operator=(const State& other) = delete;

        std::vector<SubGrid> m_subgrids;
    };
}
