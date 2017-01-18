#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "GameOfLife/Cell.h"
#include "GameOfLife/InitialState.h"
#include "GameOfLife/GameRunner.h"

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

    GameOfLife::InitialState state;

    char paren, comma;
    int64_t cellX, cellY;

    while ((in >> paren >> cellX >> comma >> cellY >> paren) && paren == ')' && comma == ',')
    {
        state.emplace_back(cellX, cellY, false);
    }

    GameOfLife::GameRunner runner(state);

    //
    // I expect a ctrl+c, yo
    //
    while (runner.Tick());

    return 0;
}
