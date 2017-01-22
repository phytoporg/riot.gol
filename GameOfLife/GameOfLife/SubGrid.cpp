#include "SubGrid.h"

#include <limits>
#include <algorithm>

#include <cassert>

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

    std::shared_ptr<uint8_t> CreateCellGrid(size_t width, size_t height)
    {
        //
        // +2 to each dimension to allow for ghost rows/columns.
        //
        // TODO: ensure that this is aligned to cache-line boundaries.
        //
        const size_t GridLength = (width + 2) * (height + 2);
        auto spGrid = std::shared_ptr<uint8_t>(new uint8_t[GridLength], std::default_delete<uint8_t[]>());

        memset(spGrid.get(), 0, GridLength);

        return spGrid;
    }
}

namespace GameOfLife
{
    SubGrid::SubGrid(const InitialState & initialState) 
        : m_generation(0)
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

        m_spCellGrid[0] = CreateCellGrid(m_width, m_height);
        m_spCellGrid[1] = CreateCellGrid(m_width, m_height);
        m_pCurrentCellGrid = m_spCellGrid[0].get();

        //
        // One more pass to initialize state...
        //

        for (const auto& cell : initialState)
        {
            RaiseCell(cell.X, cell.Y);
        }
    }

    SubGrid::SubGrid(int64_t xmin, int64_t width, int64_t ymin, int64_t height)
        : RectangularGrid(xmin, width, ymin, height),
          m_generation(0)
    {
        ValidateDimensions(m_width, m_height);

        m_spCellGrid[0] = std::make_shared<uint8_t>();
        m_spCellGrid[1] = std::make_shared<uint8_t>();

        m_spCellGrid[0] = CreateCellGrid(m_width, m_height);
        m_spCellGrid[1] = CreateCellGrid(m_width, m_height);
        m_pCurrentCellGrid = m_spCellGrid[0].get();
    }

    SubGrid::SubGrid(const SubGrid& other)
        : RectangularGrid(other.m_xMin, other.m_width, other.m_yMin, other.m_height),
          m_pCurrentCellGrid(other.m_pCurrentCellGrid)
    {
        m_spCellGrid[0] = other.m_spCellGrid[0];
        m_spCellGrid[1] = other.m_spCellGrid[1];
    }

    size_t SubGrid::GetOffset(int64_t x, int64_t y) const
    {
        x = x - m_xMin + 1;
        y = y - m_yMin + 1;

        assert(x >= 0 && y >= 0);
        assert(x < m_width + 2 && y < m_height + 2);

        return static_cast<size_t>(x + (m_width + 2) * y);
    }

    void SubGrid::RaiseCell(uint8_t* pGrid, int64_t x, int64_t y)
    {
        pGrid[GetOffset(x, y)] = true;
    }

    void SubGrid::RaiseCell(int64_t x, int64_t y)
    {
        RaiseCell(m_pCurrentCellGrid, x, y);
    }

    void SubGrid::KillCell(uint8_t* pGrid, int64_t x, int64_t y)
    {
        pGrid[GetOffset(x, y)] = false;
    }

    void SubGrid::KillCell(int64_t x, int64_t y)
    {
        KillCell(m_pCurrentCellGrid, x, y);
    }

    bool SubGrid::GetCellState(uint8_t const* pGrid, int64_t x, int64_t y) const
    {
        return !!pGrid[GetOffset(x, y)];
    }

    bool SubGrid::GetCellState(int64_t x, int64_t y) const
    {
        return GetCellState(m_pCurrentCellGrid, x, y);
    }

    void SubGrid::AdvanceGeneration()
    {
        //
        // Copy ghost rows
        //
        {
            int64_t srcY = m_yMin;
            int64_t dstY = m_yMin + m_height;
            for (int64_t x = m_xMin; x < m_xMin + m_width; x++)
            {
                m_pCurrentCellGrid[GetOffset(x, dstY)] = m_pCurrentCellGrid[GetOffset(x, srcY)];
            }

            srcY = m_yMin + m_height - 1;
            dstY = m_yMin - 1;
            for (int64_t x = m_xMin; x < m_xMin + m_width; x++)
            {
                m_pCurrentCellGrid[GetOffset(x, dstY)] = m_pCurrentCellGrid[GetOffset(x, srcY)];
            }
        }

        //
        // Copy ghost columns
        //
        {
            int64_t srcX = m_xMin;
            int64_t dstX = m_xMin + m_width;
            for (int64_t y = m_yMin; y < m_yMin + m_height; y++)
            {
                m_pCurrentCellGrid[GetOffset(dstX, y)] = m_pCurrentCellGrid[GetOffset(srcX, y)];
            }

            srcX = m_xMin + m_width - 1;
            dstX = m_xMin - 1;
            for (int64_t y = m_yMin; y < m_yMin + m_height; y++)
            {
                m_pCurrentCellGrid[GetOffset(dstX, y)] = m_pCurrentCellGrid[GetOffset(srcX, y)];
            }
        }

        decltype(m_pCurrentCellGrid) pOtherGrid =
            OtherPointer(m_pCurrentCellGrid, m_spCellGrid[0].get(), m_spCellGrid[1].get());

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

        ++m_generation;
        m_pCurrentCellGrid = pOtherGrid;
    }
}
