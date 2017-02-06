#pragma once

#include "DebugGridDumper.h"

#include <cassert>

namespace GameOfLife
{
    std::ofstream DebugGridDumper::s_fileStream;

    void DebugGridDumper::OpenFile(const std::string& filename)
    {
#if defined(DEBUG)
        if (!s_fileStream.is_open())
        {
            s_fileStream.open(filename);
            assert(s_fileStream.good());
        }
#endif
    }

    void 
    DebugGridDumper::DumpGrid(
        uint32_t generation,
        SubGrid::CoordinateType t,
        const RectangularGrid& bounds,
        uint8_t const* pBefore,
        uint8_t const* pAfter
    )
    {
#if defined(DEBUG)
        assert(s_fileStream);
        s_fileStream << std::dec << generation << std::endl;
        s_fileStream << "(" << t.first << ", " << t.second << ")" << std::endl;

        const int64_t MaxRow = bounds.Height() + 2;
        const int64_t MaxCol = bounds.Width() + 2;
        for (int64_t row = 0; row < MaxRow; row++)
        {
            for (int64_t col = 0; col < MaxCol; col++)
            {
                s_fileStream << std::hex << static_cast<int>(*(pBefore++));
                if (col < MaxCol - 1)
                {
                    s_fileStream << ",";
                }
            }

            s_fileStream << " ";

            for (int64_t col = 0; col < MaxCol; col++)
            {
                s_fileStream << std::hex << static_cast<int>(*(pAfter++));
                if (col < MaxCol - 1)
                {
                    s_fileStream << ",";
                }
            }

            s_fileStream << std::endl;
        }
#endif
    }
}

