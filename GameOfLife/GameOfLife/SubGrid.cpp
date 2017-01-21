#include "SubGrid.h"

#include <limits>
#include <algorithm>

namespace
{
    void ValidateDimensions(int64_t width, int64_t height)
    {
        if (width <= 0)
        {
            throw std::out_of_range("SubGrid width is not positive");
        }

        if (height <= 0)
        {
            throw std::out_of_range("SubGrid height is not positive");
        }
    }

    int64_t wrap(int64_t x, int64_t min, int64_t stride)
    {
        x -= min;

        if (x < 0)
        {
            x += stride;
        }
        else if (x > stride - 1)
        {
            x -= stride;
        }

        return x;
    }

    template<typename T>
    T* OtherPointer(T* pPointer, T* pOption1, T* pOption2)
    {
        return pPointer == pOption1 ? pOption2 : pOption1;
    }

    uint8_t CountNeighbors(
        const GameOfLife::SubGrid& subGrid, 
        int64_t x,
        int64_t y
        )
    {
        uint8_t neighbors = 0;

        for (int64_t dy = -1; dy <= 1; ++dy)
        {
            for (int64_t dx = -1; dx <= 1; ++dx)
            {
                if (!dx && !dy) 
                {
                    //
                    // Don't count self!
                    //
                    continue; 
                }

                if (subGrid.GetCellState(x + dx, y + dy))
                {
                    neighbors++;
                }
            }
        }

        return neighbors;
    }
}

namespace GameOfLife
{
    SubGrid::SubGrid(const InitialState & initialState)
    {
        m_xMin = std::numeric_limits<int64_t>::max();
        m_yMin = std::numeric_limits<int64_t>::max();

        //
        // Two passes to initialize: Determine dimensions of our buffer, then
        // actually initialize the state.
        //

        int64_t xMax = std::numeric_limits<int64_t>::min();
        int64_t yMax = std::numeric_limits<int64_t>::min();
        for (const auto& cell : initialState)
        {
            if (cell.X < m_xMin)
            {
                m_xMin = cell.X;
            }
            else if (cell.X > xMax)
            {
                xMax = cell.X;
            }

            if (cell.Y < m_yMin)
            {
                m_yMin = cell.Y;
            }
            else if (cell.Y > yMax)
            {
                yMax = cell.Y;
            }
        }

        m_width  = xMax - m_xMin + 1;
        m_height = yMax - m_yMin + 1;

        ValidateDimensions(m_width, m_height);

        m_cellGrid[0].resize(m_height); // TODO: This can't work -- resize() takes 32-bit integer.
        m_cellGrid[1].resize(m_height); // TODO: This can't work -- resize() takes 32-bit integer.
        for (int64_t row = 0; row < m_height; ++row)
        {
            m_cellGrid[0][row].resize(m_width, false); // TODO: ...same here
            m_cellGrid[1][row].resize(m_width, false); // TODO: ...same here
        }
        m_pCurrentCellGrid = &m_cellGrid[0];

        //
        // One more pass to initialize state...
        //

        for (const auto& cell : initialState)
        {
            RaiseCell(cell.X, cell.Y);
        }
    }

    SubGrid::SubGrid(int64_t xmin, int64_t width, int64_t ymin, int64_t height)
        : RectangularGrid(xmin, width, ymin, height)
    {
        ValidateDimensions(m_width, m_height);

        m_cellGrid[0].resize(m_height); // TODO: Nope
        m_cellGrid[1].resize(m_height); // TODO: Nope
        for (int64_t row = 0; row < m_height; ++row)
        {
            m_cellGrid[0][row].resize(m_width, false); // TODO: Same
            m_cellGrid[1][row].resize(m_width, false); // TODO: Same
        }
        m_pCurrentCellGrid = &m_cellGrid[0];
    }

    void SubGrid::RaiseCell(CellGrid* pGrid, int64_t x, int64_t y)
    {
        x = wrap(x, m_xMin, m_width);
        y = wrap(y, m_yMin, m_height);

        (*pGrid)[y][x] = true; // TODO: Operator[] takes 32-bit integer
    }

    void SubGrid::RaiseCell(int64_t x, int64_t y)
    {
        RaiseCell(m_pCurrentCellGrid, x, y);
    }

    void SubGrid::KillCell(CellGrid* pGrid, int64_t x, int64_t y)
    {
        x = wrap(x, m_xMin, m_width);
        y = wrap(y, m_yMin, m_height);

        (*pGrid)[y][x] = false; // TODO: Operator[] takes 32-bit integer
    }

    void SubGrid::KillCell(int64_t x, int64_t y)
    {
        KillCell(m_pCurrentCellGrid, x, y);
    }

    bool SubGrid::GetCellState(CellGrid const* pGrid, int64_t x, int64_t y) const
    {
        x = wrap(x, m_xMin, m_width);
        y = wrap(y, m_yMin, m_height);

        return (*pGrid)[y][x]; // TODO: Operator[] takes 32-bit integer
    }

    bool SubGrid::GetCellState(int64_t x, int64_t y) const
    {
        return GetCellState(m_pCurrentCellGrid, x, y);
    }

    void SubGrid::AdvanceGeneration()
    {
        // TODO: Use ghost buffers instead of wrapping around.
        decltype(m_pCurrentCellGrid) pOtherGrid =
            OtherPointer(m_pCurrentCellGrid, &m_cellGrid[0], &m_cellGrid[1]);

        for (int64_t y = m_yMin; y < m_yMin + m_height; y++)
        {
            for (int64_t x = m_xMin; x < m_xMin + m_width; x++)
            {
                const uint8_t NumNeighbors = CountNeighbors(*this, x, y);
                if (GetCellState(m_pCurrentCellGrid, x, y))
                {
                    if (NumNeighbors < 2 || NumNeighbors > 3)
                    {
                        KillCell(pOtherGrid, x, y);
                    }
                    else
                    {
                        RaiseCell(pOtherGrid, x, y);
                    }
                }
                else
                {
                    if (NumNeighbors == 3)
                    {
                        RaiseCell(pOtherGrid, x, y);
                    }
                    else
                    {
                        KillCell(pOtherGrid, x, y);
                    }
                }
            }
        }
    }
}
