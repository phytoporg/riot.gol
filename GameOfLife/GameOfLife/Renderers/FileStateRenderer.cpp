#include "FileStateRenderer.h"

namespace GameOfLife { namespace Renderers {

    FileStateRenderer::FileStateRenderer(const std::string& filename)
        : m_fileOut(filename)
    {}

    std::ostream& FileStateRenderer::operator<<(const SparseGrid& sparseGrid)
    {
        m_fileOut << sparseGrid;
        return m_fileOut;
    }
} }
