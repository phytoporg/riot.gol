#include "ConsoleStateRenderer.h"

#include <windows.h>

namespace GameOfLife
{
    class ConsoleStateRenderer::Pimpl
    {
    public:
        Pimpl() : m_hStdOut(GetStdHandle(STD_OUTPUT_HANDLE))
        {}

        void SetCursorPosition(int x, int y) const
        {
            CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
            GetConsoleScreenBufferInfo(m_hStdOut, &csbiInfo);
            csbiInfo.dwCursorPosition.X = x;
            csbiInfo.dwCursorPosition.Y = y;
            SetConsoleCursorPosition(m_hStdOut, csbiInfo.dwCursorPosition);
        }

        void WriteCharacter(char c)
        {
            DWORD numCharactersWritten;
            WriteConsole(m_hStdOut, &c, 1, &numCharactersWritten, nullptr);
        }

    private:
        HANDLE m_hStdOut;
    };

    ConsoleStateRenderer::ConsoleStateRenderer() : m_spPimpl(new Pimpl)
    {}

    //
    // Pimpl requires defining the destructor here.
    //
    ConsoleStateRenderer::~ConsoleStateRenderer() = default;

    void ConsoleStateRenderer::Draw(const State & state)
    {
        const int64_t xMin = state.XMin();
        const int64_t yMin = state.YMin();

        const int64_t Width = state.Width();
        const int64_t Height = state.Height();

        for (int64_t y = yMin; y < yMin + Height; ++y)
        {
            for (int64_t x = xMin; x < xMin + Width; ++x)
            {
                int cursorY = static_cast<int>(y - yMin);
                int cursorX = static_cast<int>(x - xMin);
                m_spPimpl->SetCursorPosition(cursorX, cursorY);
                
                if (state.GetCellState(x, y))
                {
                    m_spPimpl->WriteCharacter('+');
                }
                else
                {
                    m_spPimpl->WriteCharacter('-');
                }
            }
        }
    }
}
