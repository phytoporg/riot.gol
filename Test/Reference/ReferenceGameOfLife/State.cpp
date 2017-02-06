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
    State::State(const InitialState & initialCells)
        : m_xMin(std::numeric_limits<int64_t>::max()),
          m_yMin(std::numeric_limits<int64_t>::max()),
          m_generation(0)
    {
        //
        // Two passes to initialize: Determine dimensions of our buffer, then
        // actually initialize the state.
        //

        int64_t xMax = std::numeric_limits<int64_t>::min();
        int64_t yMax = std::numeric_limits<int64_t>::min();
        for (const auto& cell : initialCells)
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

        //
        // TODO: Include subgrid.h and use those dimensions, once
        // projects are merged.
        //

        //
        // Snap state to subgrid boundaries.
        //
        if (m_xMin >= 0)
        {
            m_xMin = m_xMin - (m_xMin % 30);
        }
        else
        {
            m_xMin = m_xMin - (30 + (m_xMin % 30));
        }

        if (m_yMin >= 0)
        {
            m_yMin = m_yMin - (m_yMin % 30);
        }
        else
        {
            m_yMin = m_yMin - (30 + (m_yMin % 30));
        }

        //
        // Do the same for max values
        //
        if (xMax >= 0)
        {
            xMax = xMax - (xMax % 30) + 30;
        }
        else
        {
            xMax = xMax - (30 + (xMax % 30)) + 30;
        }

        if (yMax >= 0)
        {
            yMax = yMax - (yMax % 30) + 30;
        }
        else
        {
            yMax = yMax - (30 + (yMax % 30)) + 30;
        }

        m_width = xMax - m_xMin;
        m_height = yMax - m_yMin;

        ValidateDimensions(m_width, m_height);

        m_stateBits.resize(m_height);
        for (int64_t row = 0; row < m_height; ++row)
        {
            m_stateBits[row].resize(m_width, false);
        }

        //
        // One more pass to initialize state...
        //

        for (const auto& cell : initialCells)
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

    void State::AdvanceGeneration()
    {
        m_generation++;
    }

    //
    // Outputs the minx, miny, width and height on the first line, the
    // current generation on the next line, and then dumps the state bits
    // for each cell.
    //
    std::ostream& operator<<(std::ostream& out, const State& state)
    {
        out << "(" << state.m_xMin << "," << state.m_yMin << "," << state.m_width << "," << state.m_height << ")" << '\n'
            << state.m_generation << '\n';

        const int64_t YMax = state.m_yMin + state.m_height;
        const int64_t XMax = state.m_xMin + state.m_width;
        for (int64_t y = state.m_yMin; y < YMax; y++)
        {
            for (int64_t x = state.m_xMin; x < XMax; x++)
            {
                if (state.GetCellState(x, y))
                {
                    out << "1";
                }
                else
                {
                    out << "0";
                }

                if (x < XMax - 1)
                {
                    out << ",";
                }
            }
            out << "\n";
        }

        out.flush();

        return out;
    }
}
