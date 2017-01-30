#pragma once

//
// Draws the state in a console. Only supports small states,
// just for preliminary validation.
// 

#include <GameOfLife/State.h>

#include <memory>

namespace GameOfLife
{
    namespace Renderers
    {
        class ConsoleStateRenderer
        {
        public:
            ConsoleStateRenderer();
            ~ConsoleStateRenderer();

            void Draw(const State& state);

        private:
            ConsoleStateRenderer(const ConsoleStateRenderer& other) = delete;
            ConsoleStateRenderer& operator=(const ConsoleStateRenderer& other) = delete;

            // 
            // To contain all the win32 ugliness.
            //
            class Pimpl;
            std::unique_ptr<Pimpl> m_spPimpl;
        };
    }
}
