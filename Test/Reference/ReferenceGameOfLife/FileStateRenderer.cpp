#include "FileStateRenderer.h"

namespace GameOfLife
{
    FileStateRenderer::FileStateRenderer(const std::string& filename)
        : m_fileOut(filename)
    {}

    std::ostream& FileStateRenderer::operator<<(const State& state)
    {
        if (!m_fileOut)
        {
            throw std::exception("File state renderer file is in bad shape!");
        }
        m_fileOut << state;

        return m_fileOut;
    }
}
