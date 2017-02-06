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

namespace Utility
{
    template <size_t N> class AlignedMemoryPool;
}

namespace GameOfLife
{
    class SubGridGraph;

    //
    // This class represents a single static, rectangular subregion 
    // within world space. The intent is for this class only to indirectly 
    // refer to cell grid information, which lives in the SubgridStorage
    // data structure.
    //
    class SubGrid : public RectangularGrid
    {
    public:
        //
        // Type for describing (x,y) world-space coordinates.
        //
        typedef std::pair<int64_t, int64_t> CoordinateType;

        //
        // Type for the vertex buffer ultimately used for rendering living
        // cells within the subgrid.
        //
        struct VertexType
        {
            VertexType(int64_t x, int64_t y) :
                X(static_cast<float>(x)), Y(static_cast<float>(y)) {}
            float X;
            float Y;
        };

        // 
        // All subgrids will have these logical dimensions or, in the case where
        // subgrids live on the edge of a world space whose dimensions is not an
        // even multiple of SUBGRID_WIDTH/HEIGHT, may possibly be smaller.
        //
        // A note that the actual cell grid buffers have one cell of padding in
        // each direction to store cell information from neighboring subgrids.
        //
        static const int64_t SUBGRID_WIDTH = 30;
        static const int64_t SUBGRID_HEIGHT = 30;

        //
        // Particularly helpful during initialization-- takes as input a cell
        // coordinate in world space and returns the subgrid coordinates it
        // belongs to.
        //
        // The implementation is inlined here for easy access from the reference
        // implementation. This is the only commonality at the moment; any more
        // and this can probably just be factored out into a separate library.
        //
        static int64_t SubGrid::SnapCoordinateToSubgridCorner(int64_t value, int64_t max)
        {
            int64_t newValue;

            if (value >= 0)
            {
                //
                // For positive values and zero, just remove the remainder to
                // snap back to the next least integer modulo max.
                //
                newValue = value - (value % max);
            }
            else
            {
                //
                // Negative values are a little trickier; the intent here is still
                // to get the next-least integer modulo max, but modulus doesn't
                // do what we need when operating on negative values.
                //
                newValue = ((value + 1) / max - 1) * max;
            }

            return newValue;
        }

        SubGrid(
            Utility::AlignedMemoryPool<64>& memoryPool, 
            const RectangularGrid& worldBounds,
            SubGridGraph& graph,
            int64_t xmin, int64_t ymin,
            uint32_t generation = 0
            );
        ~SubGrid();

        //
        // (x, y) coordinates are toroidal due to ghost buffers. x, y parameters
        // are in world space.
        //
        void RaiseCell(int64_t x, int64_t y);
        void KillCell(int64_t x, int64_t y);
        bool GetCellState(int64_t x, int64_t y) const;

        //
        // Retrieves this SubGrid's upper-left cell coordinates.
        //
        const CoordinateType& GetCoordinates() const;

        //
        // Retrieve this generation's vertex data for rendering. Contains
        // only living cell coordinates in world space.
        //
        const std::vector<VertexType>& GetVertexData() const;

        //
        // Returns true if any border cells are living.
        //
        bool HasBorderCells() const;
         
        //
        // Advances the cell states to the next gen and returns the number of living
        // cells produced.
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

        //
        // Sets border cells to zero.
        //
        void ClearBorder(AdjacencyIndex adjacency);

    private:
        //
        // SubGrid objects get tossed around a lot for bookkeeping, so make 
        // copies cheap. Place data on the heap; just track pointers in here.
        //
        uint8_t* m_pCellGrids[2];
        
        //
        // Reference to the memory pool for managing cell grid memory.
        // Used to allocate ping-pong cell grid buffers in the constructor,
        // frees grid buffers in the destructor.
        //
        Utility::AlignedMemoryPool<64>& m_memoryPool;

        //
        // Pointer to the active cell grid.
        //
        uint8_t* m_pCurrentCellGrid;

        //
        // Internal equivalents to the public versions above which may target
        // a particular cell grid, hence the leading parameter in each.
        //
        bool GetCellState(uint8_t const* pGrid, int64_t x, int64_t y) const;
        void RaiseCell(uint8_t* pGrid, int64_t x, int64_t y);
        void KillCell(uint8_t* pGrid, int64_t x, int64_t y);

        //
        // Copies a row from a subgrid to another.
        //
        static void CopyRowFrom(
            const SubGrid& src, uint8_t const* pSrcGrid,
            const SubGrid& dst, uint8_t* pDstGrid,
            int64_t ySrc, int64_t yDst
        );

        //
        // Zeroes out a row in the given cell grid buffer.
        //
        void ClearRow(uint8_t* pBuffer, int64_t row);

        //
        // Equivalent operations as above, but performed on columns.
        //
        static void CopyColumnFrom(
            const SubGrid& src, uint8_t const* pSrcGrid,
            const SubGrid& dst, uint8_t* pDstGrid,
            int64_t xSrc, int64_t xDst
        );
        void ClearColumn(uint8_t* pBuffer, int64_t col);

        //
        // Get offset into the current cell grid buffer where (x,y) are
        // in world coordinates.
        //
        size_t GetOffset(int64_t x, int64_t y) const;

        //
        // Starting (x,y) world-space coordinates for this subgrid.
        //
        CoordinateType m_coordinates;

        //
        // This subgrid's most recently completed generation. Should be
        // at most behind by 1 generation relative to all other subgrids.
        //
        uint32_t       m_generation;
        SubGridGraph*  m_pGridGraph;

        //
        // Subgrid width, including ghost buffer space.
        //
        int64_t m_bufferWidth;

        //
        // Subgrid height, including ghost buffer space.
        //
        int64_t m_bufferHeight;

        //
        // For rendering. Only contains set cell coordinates.
        //
        std::vector<VertexType> m_vertexData;

        //
        // For debugging and for updating values in the vertex buffer.
        //
        const RectangularGrid& m_worldBounds;
    };

    //
    // Saves some typing. These get copied around a lot, so better to manage
    // their lifetimes with smart pointers.
    //
    typedef std::shared_ptr<SubGrid> SubGridPtr;
}
