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
        SubGrid(const InitialState& initialState);
        SubGrid(int64_t xmin, int64_t width, int64_t ymin, int64_t height);

        SubGrid(const SubGrid& other);

        SubGrid& operator=(const SubGrid& other) = default;

        //
        // (x, y) coordinates are toroidal
        //
        void RaiseCell(int64_t x, int64_t y);
        void KillCell(int64_t x, int64_t y);

        bool GetCellState(int64_t x, int64_t y) const;
         
        void AdvanceGeneration();

    private:
        typedef std::vector<std::vector<bool>> CellGrid;

        //
        // Make copies cheap.
        //
        std::shared_ptr<CellGrid> m_spCellGrid[2];
        CellGrid* m_pCurrentCellGrid;

        bool GetCellState(CellGrid const* pGrid, int64_t x, int64_t y) const;
        void RaiseCell(CellGrid* pGrid, int64_t x, int64_t y);
        void KillCell(CellGrid* pGrid, int64_t x, int64_t y);
    };
}
