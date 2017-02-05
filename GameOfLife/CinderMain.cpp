#include <GameOfLife/Renderers/CinderRenderer.h>
#include <GameOfLife/Renderers/CinderRenderer_Shaders.h>
#include <GameOfLife/Renderers/FileStateRenderer.h>

#include <Utility/AlignedMemoryPool.h>

#include <cinder/gl/gl.h>
#include <cinder/app/RendererGl.h>

#include <cctype>
#include <sstream>

#include <windows.h>

using namespace cinder;

namespace
{
    std::string GetUsage(const std::string& programName)
    {
        std::stringstream ss;
        ss << "Usage: " << programName << " <filepath to initial state>" << std::endl;
        return ss.str();
    }

    void Fail(std::ostream& stream, const std::string& string)
    {
        stream << string;
        throw string;
    }
}

namespace GameOfLife
{
    namespace Renderers
    {

        CinderRenderer::CinderRenderer()
            : m_isInitialized(false),
              m_takeSingleStep(false),
              m_gameState(PAUSED),
              m_countingGenerations(false),
              m_generationsRemaining(0),
              m_memoryPool((SubGrid::SUBGRID_WIDTH + 2) * (SubGrid::SUBGRID_HEIGHT + 2), 32)
        {}

        void CinderRenderer::InitializeState(const std::vector<Cell>& cells)
        {
            m_spState.reset(new SparseGrid(cells, m_memoryPool));
            m_isInitialized = true;
        }

        void CinderRenderer::UpdateState()
        {
            if (m_countingGenerations && !m_generationsRemaining)
            {
                quit();
                return;
            }

            size_t i = 0;
            for (auto it = m_spState->begin(); it != m_spState->end(); ++it)
            {
                SubGridPtr spSubgrid = it->second;
                if (i >= m_meshes.size())
                {
                    std::vector<gl::VboMesh::Layout> layouts =
                    {
                         gl::VboMesh::Layout().usage(GL_DYNAMIC_DRAW).attrib(geom::Attrib::POSITION, 2)
                    };

                    static const size_t VertexCountPerSubgrid = SubGrid::SUBGRID_WIDTH * SubGrid::SUBGRID_HEIGHT;
                    m_meshes.push_back(
                        gl::VboMesh::create(VertexCountPerSubgrid, GL_POINTS, layouts)
                        );
                    m_meshVertexCounts.push_back(spSubgrid->GetVertexData().size());
                }
                else
                {
                    m_meshVertexCounts[i] = spSubgrid->GetVertexData().size();
                }

                auto& meshRef = m_meshes[i];
                auto vboRefs = meshRef->getVertexArrayVbos();
                assert(vboRefs.size() == 1);

                static_assert(sizeof(SubGrid::VertexType) == sizeof(glm::fvec2), "NOPE");

                auto vboRef = vboRefs.back();
                vboRef->bind();
                vboRef->bufferSubData(
                    0,
                    sizeof(SubGrid::VertexType) * m_meshVertexCounts[i],
                    spSubgrid->GetVertexData().data()
                    );
                vboRef->unbind();

                meshRef->updateNumVertices(m_meshVertexCounts[i]);

                ++i;
            }

            m_meshesToDraw = i;

            if (m_spFileStateRenderer)
            {
                *m_spFileStateRenderer << *m_spState;
            }

            if (m_countingGenerations)
            {
                m_generationsRemaining--;
            }
        }

        void CinderRenderer::keyUp(cinder::app::KeyEvent e)
        {
            const char c = std::tolower(e.getChar());
            if (c == 'p')
            {
                if (m_gameState == PAUSED)
                {
                    m_gameState = PLAYING;
                    m_takeSingleStep = false;
                }
                else
                {
                    m_gameState = PAUSED;
                }
            }
            else if (m_gameState == PAUSED && c == ' ')
            {
                m_takeSingleStep = true;
                disableFrameRate();
            }
            else if (c == 'q')
            {
                quit();
            }
        }

        void CinderRenderer::setup()
        {
            //
            // Initialize GoL data 
            //
            const auto& args = getCommandLineArgs();
            if (args.size() < 2)
            {
                Fail(console(), GetUsage(args[0]));
            }

            const std::string filename(args[1]);
            std::ifstream in(filename);
            if (!in.good())
            {
                std::stringstream ss;
                ss << "Failed to open " << filename << std::endl;
                Fail(console(), ss.str());
            }

            if (args.size() > 2)
            {
                m_countingGenerations = true;
                m_generationsRemaining = atoi(args[2].c_str());
            }

            if (args.size() > 3)
            {
                m_spFileStateRenderer.reset(new FileStateRenderer(args[3]));
            }

            std::vector<GameOfLife::Cell> cells;
            char paren, comma;
            int64_t cellX, cellY;

            while ((in >> paren >> cellX >> comma >> cellY >> paren) && paren == ')' && comma == ',')
            {
                cells.emplace_back(cellX, cellY, false);
            }

            if (cells.empty())
            {
                std::stringstream ss;
                ss << "No valid cells specified in " << filename << std::endl;
                Fail(console(), ss.str());
            }

            InitializeState(cells);

            //
            // Set up rendering parameters, shaders, etc.
            //
            gl::enableDepthRead();
            gl::enableDepthWrite();

            m_camera.setOrtho(0, 1.0f, 1.0f, 0.0f, 0.1f, 1000.0f);

            //
            // Center on the middle of the state grid.
            //
            m_camera.lookAt(vec3(0, 0, 2.0f), vec3(0));

            m_progRef = gl::GlslProg::create(gl::GlslProg::Format()
                .vertex(VERTEX_SHADER)
                .geometry(GEOMETRY_SHADER)
                .fragment(FRAGMENT_SHADER)
                );

            UpdateState();
        }

        void CinderRenderer::update()
        {
            if (!m_isInitialized)
            {
                Fail(console(), "update() called without having first been initialized.");
            }

            if (m_gameState == PAUSED && !m_takeSingleStep)
            {
                return;
            }
            m_takeSingleStep = false;

            m_spState->AdvanceGeneration();
            UpdateState();
        }
        
        void CinderRenderer::draw()
        {
            if (!m_isInitialized)
            {
                Fail(console(), "draw() called without having first been initialized.");
            }

            gl::clear(Color(0.0f, 0.0f, 0.0f));
            gl::setMatrices(m_camera);

            const float InvW = 1.0f / static_cast<float>(m_spState->Width());
            const float XMinInvW = InvW * static_cast<float>(m_spState->XMin());
            const float InvH = 1.0f / static_cast<float>(m_spState->Height());
            const float YMinInvH = InvH * static_cast<float>(m_spState->YMin());

            //
            // This is *column* major. This normalizes what is otherwise a potentially
            // very large grid and shifts coordinates to align with normalized window 
            // pixel space.
            //
            const glm::mat3 StateTransform(
                InvW,       0,        0,
                0,          InvH,     0,
                -XMinInvW, -YMinInvH, 0 // Zero here to annihilate the z-component;
                );                      // all geometry lives on the x-y plane.

            gl::setMatrices(m_camera);

            static const float CELL_WIDTH  = std::max(InvW, 0.01f) * getWindowAspectRatio();
            static const float CELL_HEIGHT = std::max(InvH, 0.01f);

            const float ElapsedTime = static_cast<float>(ci::app::getElapsedSeconds());
            const glm::vec2 Resolution(getWindowWidth(), getWindowHeight());
            for (size_t i = 0; i < m_meshesToDraw; i++)
            {
                m_progRef->uniform("uStateTransform", StateTransform);
                m_progRef->uniform("uElapsedTime", ElapsedTime);
                m_progRef->uniform("uWindowRes", Resolution);
                m_progRef->uniform("uCellWidth", CELL_WIDTH);
                m_progRef->uniform("uCellHeight", CELL_HEIGHT);
                ci::gl::BatchRef batch = gl::Batch::create(
                    m_meshes[i], m_progRef
                    );
                batch->draw();
            }
        }
    }

}

using namespace GameOfLife::Renderers;
CINDER_APP(CinderRenderer, app::RendererGl, [](cinder::app::App::Settings* pSettings)
{
    pSettings->setWindowSize(1024, 768);
    pSettings->setHighDensityDisplayEnabled();
});
