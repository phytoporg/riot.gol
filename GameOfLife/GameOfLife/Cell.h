#pragma once

#include <tuple>
#include <cstdint>

namespace GameOfLife
{
    struct Cell
    {
        Cell(int64_t x, int64_t y) : X(x), Y(y), IsAlive(false) {}
        Cell(int64_t x, int64_t y, bool alive) : X(x), Y(y), IsAlive(alive) {}

        int64_t X;
        int64_t Y;
        bool    IsAlive;
    };
}
