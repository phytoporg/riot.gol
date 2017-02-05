#pragma once

#include <fstream>
#include <string>

#include "SubGrid.h"

namespace GameOfLife
{
    //
    // Dumps subgrid state to file for debugging. Conditioned on DEBUG
    // compile-time flag.
    //
    class DebugGridDumper
    {
    public:
        static void OpenFile(const std::string& filename);
        static void DumpGrid(
            uint32_t generation,
            SubGrid::CoordinateType t,
            const RectangularGrid& bounds,
            uint8_t const* pBefore,
            uint8_t const* pAfter
        );

    private:
        static std::ofstream s_fileStream;
    };
}
