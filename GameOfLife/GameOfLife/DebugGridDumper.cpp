#pragma once

#include "DebugGridDumper.h"

#include <cassert>

namespace GameOfLife
{
    std::ofstream DebugGridDumper::s_fileStream;

    void DebugGridDumper::OpenFile(const std::string& filename)
    {
#if defined(DEBUG)
        if (!s_fileStream)
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
        uint8_t const* pBefore,
        uint8_t const* pAfter
    )
    {
#if defined(DEBUG)
        assert(s_fileStream);
        s_fileStream << std::dec << generation << std::endl;
        s_fileStream << "(" << t.first << ", " << t.second << ")" << std::endl;
        for (int64_t row = 0; row < SubGrid::SUBGRID_HEIGHT + 2; row++)
        {
            for (int64_t col = 0; col < SubGrid::SUBGRID_WIDTH + 2; col++)
            {
                s_fileStream << std::hex << static_cast<int>(*(pBefore++));
                if (col <= SubGrid::SUBGRID_WIDTH)
                {
                    s_fileStream << ",";
                }
            }

            s_fileStream << " ";

            for (int64_t col = 0; col < SubGrid::SUBGRID_WIDTH + 2; col++)
            {
                s_fileStream << std::hex << static_cast<int>(*(pAfter++));
                if (col <= SubGrid::SUBGRID_WIDTH)
                {
                    s_fileStream << ",";
                }
            }

            s_fileStream << std::endl;
        }
#endif
    }
}

