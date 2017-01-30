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
            CinderRenderer();

            virtual void setup() override;
            virtual void update() override;
            virtual void draw() override;

        private:
            void InitializeState(const std::vector<Cell>& cells);

            cinder::CameraOrtho             m_camera;

            ci::gl::GlslProgRef             m_progRef;

            std::vector<ci::gl::VboMeshRef> m_meshes;
            std::vector<size_t>             m_meshVertexCounts;
            size_t                          m_meshesToDraw;

            std::unique_ptr<State> m_spState;
            bool m_isInitialized;
        };
    }
}