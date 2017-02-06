#pragma once

// 
// The full game state. Just a 2D array for now.
//

#include "InitialState.h"
#include "Cell.h"

#include <ostream>


namespace GameOfLife
{
    class State
    {
    public:
        State(const InitialState& initialState);
        State(int64_t xmin, int64_t width, int64_t ymin, int64_t height);

        ~State() = default;

        int64_t XMin()   const { return m_xMin;   }
        int64_t Width()  const { return m_width;  }
        int64_t YMin()   const { return m_yMin;   }
        int64_t Height() const { return m_height; }

        //
        // (x, y) coordinates are toroidal
        //
        void RaiseCell(int64_t x, int64_t y);
        void KillCell(int64_t x, int64_t y);

        bool GetCellState(int64_t x, int64_t y) const;
        void AdvanceGeneration();

        friend std::ostream& operator<<(std::ostream& out, const State& state);

    private:
        State(const State& other) = delete;
        State& operator=(const State& other) = delete;

        int64_t m_xMin;
        int64_t m_width;

        int64_t m_yMin;
        int64_t m_height;

        uint32_t m_generation;

        std::vector<std::vector<bool>> m_stateBits;
    };
}
