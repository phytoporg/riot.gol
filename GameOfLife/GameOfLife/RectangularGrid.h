#pragma once

#include <cassert>
#include <cstdint>

namespace GameOfLife
{
    class RectangularGrid
    {
    public:
        RectangularGrid() : m_xMin(0), m_width(0), m_yMin(0), m_height(0) {}
        RectangularGrid(
            int64_t xMin,
            int64_t width,
            int64_t yMin,
            int64_t height
            ) : m_xMin(xMin), m_width(width), m_yMin(yMin), m_height(height) 
        {}

        int64_t XMin() const   { return m_xMin;   }
        int64_t Width() const  { return m_width;  }
        int64_t YMin() const   { return m_yMin;   }
        int64_t Height() const { return m_height; }

    protected:
        int64_t m_xMin;
        int64_t m_width;
        int64_t m_yMin;
        int64_t m_height;
    };
}