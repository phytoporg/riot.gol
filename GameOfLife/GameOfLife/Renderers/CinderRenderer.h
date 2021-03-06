#pragma once

#include <GameOfLife/SparseGrid.h>
#include <GameOfLife/Cell.h>

#include <GameOfLife/Renderers/FileStateRenderer.h>

#include <cinder/app/app.h>
#include <cinder/camera.h>
#include <cinder/gl/gl.h>

#include <memory>

//
// Forward declarations
//

namespace Utility
{
    template <size_t N> class AlignedMemoryPool;
}

namespace GameOfLife
{
    namespace Renderers
    {
        class CinderRenderer : public cinder::app::App
        {
        public:
            CinderRenderer();

            virtual void setup() override;
            virtual void update() override;
            virtual void draw() override;
            virtual void keyUp(cinder::app::KeyEvent e) override;

        private:
            enum GameState
            {
                PAUSED,
                PLAYING,
                MAX
            };

            //
            // Memory pool must be destroyed *after* game state is destroyed, 
            // so ensure  that it is declared beforehand (destructor removes 
            // objects in reverse order of declaration).
            //
            Utility::AlignedMemoryPool<64> m_memoryPool;
            GameState m_gameState;

            void InitializeState(const std::vector<Cell>& cells);
            void UpdateState();

            cinder::CameraOrtho             m_camera;

            ci::gl::GlslProgRef             m_progRef;

            std::vector<ci::gl::VboMeshRef> m_meshes;
            std::vector<size_t>             m_meshVertexCounts;
            size_t                          m_meshesToDraw;

            std::unique_ptr<SparseGrid> m_spState;
            bool m_isInitialized;
            bool m_takeSingleStep;

            std::unique_ptr<FileStateRenderer> m_spFileStateRenderer;
            bool m_countingGenerations;
            int32_t m_generationsRemaining;
        };
    }
}