#pragma once

#include <GameOfLife/State.h>
#include <GameOfLife/Cell.h>

#include <cinder/app/app.h>
#include <cinder/camera.h>
#include <cinder/gl/gl.h>

#include <memory>

namespace GameOfLife
{
    namespace Renderers
    {
        class CinderRenderer : public cinder::app::App
        {
        public:
            static const float DefaultFramerate;

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

            GameState m_gameState;

            void InitializeState(const std::vector<Cell>& cells);
            void UpdateState();

            cinder::CameraOrtho             m_camera;

            ci::gl::GlslProgRef             m_progRef;

            std::vector<ci::gl::VboMeshRef> m_meshes;
            std::vector<size_t>             m_meshVertexCounts;
            size_t                          m_meshesToDraw;

            std::unique_ptr<State> m_spState;
            bool m_isInitialized;

            bool m_takeSingleStep;
        };
    }
}