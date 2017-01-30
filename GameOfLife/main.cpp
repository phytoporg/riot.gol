// TODO: REMOVE
#if 0
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cctype> // tolower

#include "GameOfLife/Cell.h"
#include "GameOfLife/Renderers/ConsoleStateRenderer.h"

#include "GameOfLife/Renderers/CinderRenderer.h"

void PrintUsage(const std::string& programName)
{
    std::cerr << "Usage: " << programName << " <filepath to initial state>" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        PrintUsage(argv[0]);
        return -1;
    }

    const std::string filename(argv[1]);
    std::ifstream in(filename);
    if (!in.good())
    {
        std::cerr << "Failed to open " << filename << std::endl;
        return -1;
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
        std::cerr << "No valid cells specified in " << filename << std::endl;
        return -1;
    }

    GameOfLife::State state(cells);

    GameOfLife::Renderers::CinderRenderer crender;

    //
    // I expect a ctrl+c, yo. TODO: handle that sigint
    //
    GameOfLife::Renderers::ConsoleStateRenderer renderer;
    do
    {
        renderer.Draw(state);

        char c = std::cin.get();
        if (std::tolower(c) == 'q')
        {
            break;
        }
    } while (state.AdvanceGeneration());

    return 0;
}
#endif
