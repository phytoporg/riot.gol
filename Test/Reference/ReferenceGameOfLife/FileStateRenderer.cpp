#include "FileStateRenderer.h"

namespace GoLReference
{
    FileStateRenderer::FileStateRenderer(const std::string& filename)
        : m_fileOut(filename)
    {}

    FileStateRenderer::~FileStateRenderer()
    {
        m_fileOut.flush();
    }

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
