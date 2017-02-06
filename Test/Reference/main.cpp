#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "ReferenceGameOfLife/Cell.h"
#include "ReferenceGameOfLife/InitialState.h"
#include "ReferenceGameOfLife/GameRunner.h"
#include "ReferenceGameOfLife/FileStateRenderer.h"

void PrintUsage(const std::string& programName)
{
    std::cerr << "Usage: " << programName << " <filepath to initial state> <# generations> <output path>" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 4)
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

    int32_t generationsRemaining = atoi(argv[2]);
    std::string outputFilename(argv[3]);

    GoLReference::InitialState state;

    char paren, comma;
    int64_t cellX, cellY;

    while ((in >> paren >> cellX >> comma >> cellY >> paren) && paren == ')' && comma == ',')
    {
        state.emplace_back(cellX, cellY, false);
    }

    GoLReference::GameRunner runner(state);

    GoLReference::FileStateRenderer renderer(outputFilename);
    do
    {
        const auto& state = runner.CurrentState();
        renderer << state;
        runner.Tick();

        --generationsRemaining;
    } while(generationsRemaining > 0);

    return 0;
}
