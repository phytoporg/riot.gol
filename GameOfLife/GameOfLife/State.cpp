#include "State.h"

#include <limits>
#include <algorithm>

namespace
{
    void ValidateDimensions(int64_t width, int64_t height)
    {
        if (width <= 0)
        {
            throw std::out_of_range("State width is not positive");
        }

        if (height <= 0)
        {
            throw std::out_of_range("State height is not positive");
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
}

namespace GameOfLife
{
    State::State(const InitialState & initialState)
        : m_xMin(std::numeric_limits<int64_t>::max()),
          m_yMin(std::numeric_limits<int64_t>::max())
    {
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

        m_width = xMax - m_xMin + 1;
        m_height = yMax - m_yMin + 1;

        ValidateDimensions(m_width, m_height);

        m_stateBits.resize(m_height); // TODO: This can't work -- resize() takes 32-bit integer.
        for (int64_t row = 0; row < m_height; ++row)
        {
            m_stateBits[row].resize(m_width, false); // TODO: ...same here
        }

        //
        // One more pass to initialize state...
        //

        for (const auto& cell : initialState)
        {
            RaiseCell(cell.X, cell.Y);
        }
    }

    State::State(int64_t xmin, int64_t width, int64_t ymin, int64_t height)
        : m_xMin(xmin), m_width(width), m_yMin(ymin), m_height(height)
    {
        ValidateDimensions(m_width, m_height);

        m_stateBits.resize(m_height); // TODO: Nope
        for (int64_t row = 0; row < m_height; ++row)
        {
            m_stateBits[row].resize(m_width, false); // TODO: Same
        }
    }

    void State::RaiseCell(int64_t x, int64_t y)
    {
        x = wrap(x, m_xMin, m_width);
        y = wrap(y, m_yMin, m_height);

        m_stateBits[y][x] = true; // TODO: Operator[] takes 32-bit integer
    }

    void State::KillCell(int64_t x, int64_t y)
    {
        x = wrap(x, m_xMin, m_width);
        y = wrap(y, m_yMin, m_height);

        m_stateBits[y][x] = false; // TODO: Operator[] takes 32-bit integer
    }

    bool State::GetCellState(int64_t x, int64_t y) const
    {
        x = wrap(x, m_xMin, m_width);
        y = wrap(y, m_yMin, m_height);

        return m_stateBits[y][x]; // TODO: Operator[] takes 32-bit integer
    }
}
