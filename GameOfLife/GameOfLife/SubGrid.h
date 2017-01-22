#pragma once

// 
// A grid of cells which represents a sub-region of the whole game
// state.
//

#include "InitialState.h"
#include "RectangularGrid.h"

#include <memory>

namespace GameOfLife
{
    class SubGrid : public RectangularGrid
    {
    public:
        typedef std::pair<int64_t, int64_t> CoordinateType;

        SubGrid(const InitialState& initialState);
        SubGrid(int64_t xmin, int64_t width, int64_t ymin, int64_t height);

        SubGrid(const SubGrid& other);

        //
        // For STL containers.
        //
        SubGrid& operator=(const SubGrid& other) = default;

        //
        // (x, y) coordinates are toroidal
        //
        void RaiseCell(int64_t x, int64_t y);
        void KillCell(int64_t x, int64_t y);

        bool GetCellState(int64_t x, int64_t y) const;
         
        void AdvanceGeneration();
        uint64_t GetGeneration() const { return m_generation; }

    private:
        //
        // Make copies cheap by putting everything on the heap; just track
        // pointers.
        //
        std::shared_ptr<uint8_t> m_spCellGrid[2];
        uint8_t* m_pCurrentCellGrid;

        bool GetCellState(uint8_t const* pGrid, int64_t x, int64_t y) const;
        void RaiseCell(uint8_t* pGrid, int64_t x, int64_t y);
        void KillCell(uint8_t* pGrid, int64_t x, int64_t y);

        size_t GetOffset(int64_t x, int64_t y) const;

        uint64_t m_generation;
    };
}
