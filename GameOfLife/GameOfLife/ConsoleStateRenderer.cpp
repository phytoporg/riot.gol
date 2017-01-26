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

        void WriteCharacter(char c, WORD color = 0)
        {
            if (color)
            {
                SetConsoleTextAttribute(m_hStdOut, color);
            }
            else
            {
                SetConsoleTextAttribute(m_hStdOut, 7);
            }

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

    void ConsoleStateRenderer::Draw(const State& state)
    {
        //
        // Make a first pass and clear out the whole grid, then set cells
        // which are alive according to the subgrid values.
        //
        {
            const int64_t xMin = state.XMin();
            const int64_t yMin = state.YMin();

            for (int64_t y = yMin; y < yMin + state.Height(); y++)
            {
                for (int64_t x = xMin; x < xMin + state.Width(); x++)
                {
                    int cursorX = static_cast<int>(x - xMin);
                    int cursorY = static_cast<int>(y - yMin);
                    m_spPimpl->SetCursorPosition(cursorX, cursorY);
                    m_spPimpl->WriteCharacter('-');
                }
            }
        }


        static const WORD ColorBuffer[] = { BACKGROUND_BLUE, BACKGROUND_GREEN, BACKGROUND_RED, BACKGROUND_RED };
        int colorIndex = 0;

        const auto& subgrids = state.GetSubgrids();
        for (const auto& subgrid : subgrids)
        {
            const int64_t xMin = subgrid.XMin();
            const int64_t yMin = subgrid.YMin();

            for (int64_t y = yMin; y < yMin + subgrid.Height(); ++y)
            {
                for (int64_t x = xMin; x < xMin + subgrid.Width(); ++x)
                {
                    if (subgrid.GetCellState(x, y))
                    {
                        int cursorX = static_cast<int>(x - state.XMin());
                        int cursorY = static_cast<int>(y - state.YMin());
                        m_spPimpl->SetCursorPosition(cursorX, cursorY);
                        m_spPimpl->WriteCharacter('+', ColorBuffer[colorIndex]);
                    }
                }
            }

            colorIndex = (colorIndex + 1) % ARRAYSIZE(ColorBuffer);
        }

        m_spPimpl->SetCursorPosition(
            static_cast<int>(state.Width()),
            static_cast<int>(state.Height())
            );
        m_spPimpl->WriteCharacter(' '); // Resets the color for next round.
    }
}
