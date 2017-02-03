#pragma once

// 
// A grid of cells which represents a sub-region of the whole game
// state.
//

#include "RectangularGrid.h"
#include "AdjacencyIndex.h"

#include <Utility/AlignedMemoryPool.h>

#include <memory>
#include <vector>

namespace GameOfLife
{
    class SubGridGraph;

    class SubGrid : public RectangularGrid
    {
    public:
        typedef std::pair<int64_t, int64_t> CoordinateType;

        struct VertexType
        {
            VertexType(int64_t x, int64_t y) :
                X(static_cast<float>(x)), Y(static_cast<float>(y)) {}
            float X;
            float Y;
        };

        static const int64_t SUBGRID_WIDTH = 30;
        static const int64_t SUBGRID_HEIGHT = 30;

        SubGrid();
        SubGrid(
            uint8_t* ppGrids[2],
            SubGridGraph& graph,
            int64_t xmin, int64_t width,
            int64_t ymin, int64_t height,
            uint32_t generation = 0
            );

        //
        // Free resources on removal.
        //
        template<size_t N>
        void Cleanup(Utility::AlignedMemoryPool<N>& pool)
        {
            pool.Free(m_pCellGrids[0]);
            m_pCellGrids[0] = nullptr;
            pool.Free(m_pCellGrids[1]);
            m_pCellGrids[1] = nullptr;
        }

        //
        // (x, y) coordinates are somewhat toroidal due to ghost buffers.
        //
        void RaiseCell(int64_t x, int64_t y);
        void KillCell(int64_t x, int64_t y);

        bool GetCellState(int64_t x, int64_t y) const;
        const CoordinateType& GetCoordinates() const;
        const std::vector<VertexType>& GetVertexData() const;

        bool HasBorderCells() const;
         
        //
        // Returns the number of cells alive in the next generation
        //
        uint32_t AdvanceGeneration();
        uint32_t GetGeneration() const { return m_generation; }

        //
        // Determines if the next generation will impact cells in a neighbor
        // which does not yet exist.
        //
        bool IsNextGenerationNeighbor(AdjacencyIndex adjacency) const;

        //
        // Copies border cells of a neighbor according to its adjacency.
        //
        void CopyBorder(const SubGrid& other, AdjacencyIndex adjacency);
        void PublishBorder(SubGrid& other, AdjacencyIndex adjacency);
        void ClearBorder(AdjacencyIndex adjacency);

    private:
        //
        // SubGrid objects get tossed around a lot for bookkeeping, so make 
        // copies cheap. Place data on the heap; just track pointers in here.
        //
        uint8_t* m_pCellGrids[2];
        uint8_t* m_pCurrentCellGrid;

        bool GetCellState(uint8_t const* pGrid, int64_t x, int64_t y) const;
        void RaiseCell(uint8_t* pGrid, int64_t x, int64_t y);
        void KillCell(uint8_t* pGrid, int64_t x, int64_t y);

        static void CopyRowFrom(
            const SubGrid& src, uint8_t const* pSrcGrid,
            const SubGrid& dst, uint8_t* pDstGrid,
            int64_t ySrc, int64_t yDst
        );
        void ClearRow(uint8_t* pBuffer, int64_t row);
        static void CopyColumnFrom(
            const SubGrid& src, uint8_t const* pSrcGrid,
            const SubGrid& dst, uint8_t* pDstGrid,
            int64_t xSrc, int64_t xDst
        );
        void ClearColumn(uint8_t* pBuffer, int64_t col);

        size_t GetOffset(int64_t x, int64_t y) const;

        CoordinateType m_coordinates;
        uint32_t       m_generation;
        SubGridGraph*  m_pGridGraph;

        int64_t m_bufferWidth;
        int64_t m_bufferHeight;

        //
        // For rendering. Only contains set cell coordinates.
        //
        std::vector<VertexType> m_vertexData;
    };
}
