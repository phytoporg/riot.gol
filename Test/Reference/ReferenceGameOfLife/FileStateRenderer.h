#pragma once

//
// Renders the state into a file. This will be used
// to evaluate the subgrid implementation.
//

#include "State.h"

#include <string>
#include <fstream>

namespace GameOfLife
{
    class FileStateRenderer
    {
    public:
        FileStateRenderer(const std::string& filename);
        std::ostream& operator<<(const State& state);

    private:
        std::ofstream m_fileOut;
    };
}
