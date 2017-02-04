#pragma once

//
// This class renders to a file, primarily for validation purposes.
//

#include "GameOfLife\SparseGrid.h"

#include <string>
#include <fstream>

namespace GameOfLife
{
    namespace Renderers
    {
        class FileStateRenderer
        {
        public:
            FileStateRenderer(const std::string& filename);

            std::ostream& operator<<(const SparseGrid& sparseGrid);

        private:
            std::ofstream m_fileOut;
        };
    }
}