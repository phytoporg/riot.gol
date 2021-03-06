#include "SubGrid.h"
#include "SubgridGraph.h"

#include "DebugGridDumper.h"

#include <limits>
#include <algorithm>

#include <cassert>

#include <windows.h>
#include <sstream>

namespace
{
    template<typename T>
    T* OtherPointer(T* pPointer, T* pOption1, T* pOption2)
    {
        return pPointer == pOption1 ? pOption2 : pOption1;
    }

    uint8_t CountNeighbors(
        const GameOfLife::SubGrid& subGrid, 
        int64_t x,
        int64_t y
        )
    {
        using namespace GameOfLife;
        uint8_t neighbors = 0;

        for (int i = 0; i < GameOfLife::AdjacencyIndex::MAX; ++i)
        {
            const auto Adjacency = static_cast<AdjacencyIndex>(i);
            SubGrid::CoordinateType offset = SubGridGraph::GetNeighborPositionFromIndex(Adjacency);
            if (subGrid.GetCellState(x + offset.first, y + offset.second))
            {
                neighbors++;
            }
        }

        return neighbors;
    }
}

namespace GameOfLife
{
    SubGrid::SubGrid(
        Utility::AlignedMemoryPool<64>& memoryPool,
        const RectangularGrid& worldBounds,
        SubGridGraph& graph, 
        int64_t xmin, int64_t ymin,
        uint32_t generation
        )
        : RectangularGrid(xmin, SubGrid::SUBGRID_WIDTH, ymin, SubGrid::SUBGRID_HEIGHT),
          m_generation(generation),
          m_pGridGraph(&graph),
          m_memoryPool(memoryPool),
          m_worldBounds(worldBounds)
    {
        m_vertexData.reserve(SUBGRID_WIDTH * SUBGRID_HEIGHT);

        //
        // +2 to make room for ghost buffers.
        //
        m_bufferWidth  = m_width + 2;
        m_bufferHeight = m_height + 2;

        m_pCellGrids[0] = m_memoryPool.Allocate();
        m_pCellGrids[1] = m_memoryPool.Allocate();
        m_pCurrentCellGrid = m_pCellGrids[0];

        m_coordinates = std::make_pair(m_xMin, m_yMin);
    }

    SubGrid::~SubGrid()
    {
        m_memoryPool.Free(m_pCellGrids[0]);
        m_pCellGrids[0] = nullptr;
        m_memoryPool.Free(m_pCellGrids[1]);
        m_pCellGrids[1] = nullptr;
    }

    size_t SubGrid::GetOffset(int64_t x, int64_t y) const
    {
        x = x - m_xMin + 1;
        y = y - m_yMin + 1;

        assert(x >= 0 && y >= 0);
        assert(x < m_bufferWidth && y < m_bufferHeight);

        return static_cast<size_t>(x + m_bufferWidth * y);
    }

    void SubGrid::RaiseCell(uint8_t* pGrid, int64_t x, int64_t y)
    {
        pGrid[GetOffset(x, y)] = true;

        //
        // This should only be called when updating state; so we know to add a vertex
        // here. It's assumed that this buffer is cleared out when advancing generations.
        //
        m_vertexData.emplace_back(x - m_worldBounds.XMin(), y - m_worldBounds.YMin());
    }

    void SubGrid::RaiseCell(int64_t x, int64_t y)
    {
        RaiseCell(m_pCurrentCellGrid, x, y);
    }

    void SubGrid::KillCell(uint8_t* pGrid, int64_t x, int64_t y)
    {
        pGrid[GetOffset(x, y)] = false;
    }

    void SubGrid::KillCell(int64_t x, int64_t y)
    {
        KillCell(m_pCurrentCellGrid, x, y);
    }

    bool SubGrid::GetCellState(uint8_t const* pGrid, int64_t x, int64_t y) const
    {
        return !!pGrid[GetOffset(x, y)];
    }

    bool SubGrid::GetCellState(int64_t x, int64_t y) const
    {
        return GetCellState(m_pCurrentCellGrid, x, y);
    }

    const SubGrid::CoordinateType& SubGrid::GetCoordinates() const
    {
        return m_coordinates;
    }

    const std::vector<SubGrid::VertexType>& SubGrid::GetVertexData() const
    {
        return m_vertexData;
    }

    void 
    SubGrid::CopyRowFrom(
        const SubGrid& src, uint8_t const* pSrcGrid,
        const SubGrid& dst, uint8_t* pDstGrid,
        int64_t ySrc,
        int64_t yDst
        )
    {
        assert(dst.m_width == src.m_width);
        const uint8_t* const pSrc = &pSrcGrid[src.GetOffset(src.m_xMin, ySrc)];
              uint8_t* const pDst = &pDstGrid[dst.GetOffset(dst.m_xMin, yDst)];

        memcpy(pDst, pSrc, dst.m_width);
    }

    void SubGrid::ClearRow(uint8_t* pBuffer, int64_t row)
    {
        uint8_t* const pDst = &pBuffer[GetOffset(m_xMin, row)];
        memset(pDst, 0, m_width);
    }

    void SubGrid::CopyColumnFrom(
        const SubGrid& src, uint8_t const* pSrcGrid,
        const SubGrid& dst, uint8_t* pDstGrid,
        int64_t xSrc,
        int64_t xDst
        )
    {
        assert(dst.m_height == src.m_height);
        const uint8_t* pSrc = &pSrcGrid[src.GetOffset(xSrc, src.m_yMin)];
              uint8_t* pDst = &pDstGrid[dst.GetOffset(xDst, dst.m_yMin)];

        for (int64_t i = 0; i < dst.m_height; i++)
        {
            *pDst = *pSrc;
            pDst += dst.m_bufferWidth;
            pSrc += src.m_bufferWidth;
        }
    }

    void SubGrid::ClearColumn(
        uint8_t* pBuffer, int64_t col
        )
    {
        uint8_t* pDst = &pBuffer[GetOffset(col, m_yMin)];

        for (int64_t i = 0; i < m_height; i++)
        {
            *pDst = 0;
            pDst += m_bufferWidth;
        }
    }

    bool SubGrid::HasBorderCells() const
    {
        //
        // We use this type to process as many cells as possible per
        // loop iteration, casting the byte type to an int64_t and 
        // checking against 0.
        //
        // That optimization is only available for rows, not columns,
        // whose elements aren't contiguous in memory.
        //

        typedef int64_t SkipPtrType;

        //
        // Check top row first
        //
        uint8_t* pTemp = &m_pCurrentCellGrid[GetOffset(m_xMin - 1, m_yMin - 1)];
        uint64_t i = 0;
        for (i = 0; i < m_bufferWidth / sizeof(SkipPtrType); i++)
        {
            auto pSkipTemp = reinterpret_cast<SkipPtrType*>(pTemp);
            if (*pSkipTemp) return true;
            pTemp += sizeof(SkipPtrType);
        }

        if (i * sizeof(SkipPtrType) < m_bufferWidth)
        {
            const uint64_t Remaining = m_bufferWidth % sizeof(SkipPtrType);
            for (i = 0; i < Remaining; i++)
            {
                if (*(++pTemp))
                {
                    return true;
                }
            }
        }

        //
        // Then the bottom
        //
        pTemp = &m_pCurrentCellGrid[GetOffset(m_xMin - 1, m_yMin + m_height)];
        for (int64_t i = 0; i < m_bufferWidth / sizeof(SkipPtrType); i++)
        {
            auto pSkipTemp = reinterpret_cast<SkipPtrType*>(pTemp);
            if (*pSkipTemp) return true;
            pTemp += sizeof(SkipPtrType);
        }

        if (i * sizeof(SkipPtrType) < m_bufferHeight)
        {
            const uint64_t Remaining = m_bufferHeight % sizeof(SkipPtrType);
            for (i = 0; i < Remaining; i++)
            {
                if (*(++pTemp))
                {
                    return true;
                }
            }
        }

        //
        // Left
        //
        pTemp = &m_pCurrentCellGrid[GetOffset(m_xMin - 1, m_yMin - 1)];
        for (int64_t i = 0; i < m_bufferHeight; i++)
        {
            if (*pTemp)
            {
                return true;
            }

            pTemp += m_bufferWidth;
        }

        //
        // Right
        //
        pTemp = &m_pCurrentCellGrid[GetOffset(m_xMin + m_width, m_yMin - 1)];
        for (int64_t i = 0; i < m_bufferHeight; i++)
        {
            if (*pTemp)
            {
                return true;
            }

            pTemp += m_bufferWidth;
        }

        return false;
    }

    uint32_t SubGrid::AdvanceGeneration()
    {
        //
        // Copy neighbor border data if it exists.
        //
        SubGrid** ppNeighbors;
        if (!m_pGridGraph->GetNeighborArray(this, ppNeighbors))
        {
            assert(false);
            throw "Subgrid exists but isn't in the grid graph!";
        }

        for (int i = 0; i < AdjacencyIndex::MAX; ++i)
        {
            SubGrid* pNeighbor = ppNeighbors[i];
            if (!pNeighbor)
            {
                continue;
            }

            CopyBorder(*pNeighbor, static_cast<AdjacencyIndex>(i));
        }

        uint8_t* pOtherGrid =
            OtherPointer(
                m_pCurrentCellGrid,
                m_pCellGrids[0],
                m_pCellGrids[1]
                );

        m_vertexData.clear();

        for (int64_t y = m_yMin; y < m_yMin + m_height; y++)
        {
            for (int64_t x = m_xMin; x < m_xMin + m_width; x++)
            {
                const uint8_t NumNeighbors = CountNeighbors(*this, x, y);
                if (GetCellState(m_pCurrentCellGrid, x, y))
                {
                    if (NumNeighbors < 2 || NumNeighbors > 3)
                    {
                        KillCell(pOtherGrid, x, y);
                    }
                    else
                    {
                        RaiseCell(pOtherGrid, x, y);
                    }
                }
                else
                {
                    if (NumNeighbors == 3)
                    {
                        RaiseCell(pOtherGrid, x, y);
                    }
                    else
                    {
                        KillCell(pOtherGrid, x, y);
                    }
                }
            }
        }

        ++m_generation;
        m_pCurrentCellGrid = pOtherGrid;

        //
        // Need to copy border states to new generation grid here as well to correctly
        // identify new neighbors after this generation has completed.
        //
        for (int i = 0; i < AdjacencyIndex::MAX; ++i)
        {
            SubGrid* pNeighbor = ppNeighbors[i];
            if (!pNeighbor)
            {
                continue;
            }

            CopyBorder(*pNeighbor, static_cast<AdjacencyIndex>(i));
        }

        DebugGridDumper::OpenFile("grid_dump.txt");
        DebugGridDumper::DumpGrid(
            m_generation - 1,
            GetCoordinates(),
            *this,
            OtherPointer(m_pCurrentCellGrid, m_pCellGrids[0], m_pCellGrids[1]),
            m_pCurrentCellGrid
        );

        return static_cast<uint32_t>(m_vertexData.size());
    }

    bool SubGrid::IsNextGenerationNeighbor(AdjacencyIndex adjacency) const
    {
        SubGrid** ppNeighbors;
        if (!m_pGridGraph->GetNeighborArray(this, ppNeighbors))
        {
            assert(false);
            throw "Subgrid exists but isn't in the grid graph!";
        }

        if (ppNeighbors[adjacency])
        {
            return false;
        }

        const int64_t BufferLeftX   = m_xMin - 1;
        const int64_t BufferRightX  = m_xMin + m_width;
        const int64_t BufferTopY    = m_yMin - 1;
        const int64_t BufferBottomY = m_yMin + m_height;

        const int64_t LeftX   = m_xMin;
        const int64_t RightX  = m_xMin + m_width - 1;
        const int64_t TopY    = m_yMin;
        const int64_t BottomY = m_yMin + m_height - 1;

        switch (adjacency)
        {
        //
        // Cheating a little here, since the corner cases have many caveats and to check
        // for absolute necessity would mean to look at non-neighboring subgrids as well.
        // Take a conservative strategy here and create a corner neighbor if any cells
        // surrounding it are live.
        //
        case GameOfLife::TOP_LEFT:
            return GetCellState(BufferLeftX, TopY) || 
                   GetCellState(LeftX, TopY) ||
                   GetCellState(LeftX, BufferTopY);
        case GameOfLife::TOP_RIGHT:
            return GetCellState(BufferRightX, TopY) || 
                   GetCellState(RightX, TopY) ||
                   GetCellState(BufferRightX, BufferTopY);
        case GameOfLife::BOTTOM_LEFT:
            return GetCellState(BufferLeftX, BottomY) || 
                   GetCellState(LeftX, BottomY) ||
                   GetCellState(LeftX, BufferBottomY);
        case GameOfLife::BOTTOM_RIGHT:
            return GetCellState(BufferRightX, BottomY) || 
                   GetCellState(RightX, BottomY) ||
                   GetCellState(RightX, BufferTopY);
        //
        // For strict cardinal directions, it is sufficient to determine whether or not a
        // new neighbor needs to exist if three consecutive live cells are found, 
        // including the ghost buffers.
        //
        case GameOfLife::TOP:
        {
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(BufferLeftX, TopY)];
            uint32_t numConsecutiveLivingCells = 0;
            for (int64_t i = 0; i < m_bufferWidth; i++)
            {
                if (*pStart)
                {
                    numConsecutiveLivingCells++;
                }
                else
                {
                    numConsecutiveLivingCells = 0;
                }

                if (numConsecutiveLivingCells == 3)
                {
                    return true;
                }

                pStart++;
            }
        }
            break;
        case GameOfLife::BOTTOM:
        {
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(BufferLeftX, BottomY)];
            uint32_t consecutiveLivingCells = 0;
            for (int64_t i = 0; i < m_bufferWidth; i++)
            {
                if (*pStart)
                {
                    consecutiveLivingCells++;
                }
                else
                {
                    consecutiveLivingCells = 0;
                }

                if (consecutiveLivingCells == 3)
                {
                    return true;
                }

                pStart++;
            }
        }
            break;

        case GameOfLife::LEFT:
        {
            uint8_t consecutiveLivingCells = 0;
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(LeftX, BufferTopY)];
            for (int64_t i = 0; i < m_bufferHeight; i++)
            {
                if (*pStart)
                {
                    ++consecutiveLivingCells;
                }
                else
                {
                    consecutiveLivingCells = 0;
                }

                if (consecutiveLivingCells == 3)
                {
                    return true;
                }

                pStart += m_bufferWidth;
            }
        }
            break;
        case GameOfLife::RIGHT:
        {
            uint8_t consecutiveLivingCells = 0;
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(RightX, BufferTopY)];
            for (int64_t i = 0; i < m_bufferHeight; i++)
            {
                if (*pStart)
                {
                    ++consecutiveLivingCells;
                }
                else
                {
                    consecutiveLivingCells = 0;
                }

                if (consecutiveLivingCells == 3)
                {
                    return true;
                }

                pStart += m_bufferWidth;
            }
        }
            break;
        default:
            break;
        }

        assert(adjacency >= 0);
        assert(adjacency < AdjacencyIndex::MAX);

        return false;
    }

    void SubGrid::CopyBorder(const SubGrid& other, AdjacencyIndex adjacency)
    {
        //
        // Use the proper grid pointer for our neighbor's data. TODO: 
        // interface should refer to generation rather than checking pointers
        // like this. This is clunky.
        //
        uint8_t* pNeighborGrid = other.m_pCurrentCellGrid;
        if (other.m_generation > m_generation)
        {
            pNeighborGrid =
                OtherPointer(
                    pNeighborGrid,
                    other.m_pCellGrids[0],
                    other.m_pCellGrids[1]
                    );
        }

        switch (adjacency)
        {
        case AdjacencyIndex::TOP_LEFT:
            m_pCurrentCellGrid[GetOffset(m_xMin - 1, m_yMin - 1)] = 
                other.GetCellState(
                    pNeighborGrid,
                    other.XMin() + other.Width()  - 1,
                    other.YMin() + other.Height() - 1
                    );
            break;
        case AdjacencyIndex::TOP:
            CopyRowFrom(
                other, pNeighborGrid,
                *this, m_pCurrentCellGrid,
                other.YMin() + other.Height() - 1,
                m_yMin - 1
                );
            break;
        case AdjacencyIndex::TOP_RIGHT:
            m_pCurrentCellGrid[GetOffset(m_xMin + m_width, m_yMin - 1)] = 
                other.GetCellState(
                    pNeighborGrid,
                    other.XMin(),
                    other.YMin() + other.Height() - 1
                    );
            break;
        case AdjacencyIndex::LEFT:
            CopyColumnFrom(
                other, pNeighborGrid,
                *this, m_pCurrentCellGrid,
                other.XMin() + other.Width() - 1,
                m_xMin - 1
                );
            break;
        case AdjacencyIndex::RIGHT:
            CopyColumnFrom(
                other, pNeighborGrid,
                *this, m_pCurrentCellGrid,
                other.XMin(),
                m_xMin + m_width
                );
            break;
        case AdjacencyIndex::BOTTOM_LEFT:
            m_pCurrentCellGrid[GetOffset(m_xMin - 1, m_yMin + m_height)] = 
                other.GetCellState(
                    pNeighborGrid,
                    other.XMin() + other.Width() - 1,
                    other.YMin()
                    );
            break;
        case AdjacencyIndex::BOTTOM:
            CopyRowFrom(
                other, pNeighborGrid,
                *this, m_pCurrentCellGrid,
                other.YMin(),
                m_yMin + m_height
                );
            break;
        case AdjacencyIndex::BOTTOM_RIGHT:
            m_pCurrentCellGrid[GetOffset(m_xMin + m_width, m_yMin + m_height)] = 
                other.GetCellState(
                    pNeighborGrid,
                    other.XMin(),
                    other.YMin()
                    );
            break;
        default:
            assert(false);
            break;
        }
    }

    void SubGrid::ClearBorder(AdjacencyIndex adjacency)
    {
        uint8_t* pOtherGrid =
            OtherPointer(
                m_pCurrentCellGrid,
                m_pCellGrids[0],
                m_pCellGrids[1]
                );
        switch (adjacency)
        {
        case AdjacencyIndex::TOP_LEFT:
            m_pCellGrids[0][GetOffset(m_xMin - 1, m_yMin - 1)] = 0;
            m_pCellGrids[1][GetOffset(m_xMin - 1, m_yMin - 1)] = 0;
            break;
        case AdjacencyIndex::TOP:
            ClearRow(m_pCellGrids[0], m_yMin - 1);
            ClearRow(m_pCellGrids[1], m_yMin - 1);
            break;
        case AdjacencyIndex::TOP_RIGHT:
            m_pCellGrids[0][GetOffset(m_xMin + m_width, m_yMin - 1)] = 0;
            m_pCellGrids[1][GetOffset(m_xMin + m_width, m_yMin - 1)] = 0;
            break;
        case AdjacencyIndex::LEFT:
            ClearColumn(m_pCellGrids[0], m_xMin - 1);
            ClearColumn(m_pCellGrids[1], m_xMin - 1);
            break;
        case AdjacencyIndex::RIGHT:
            ClearColumn(m_pCellGrids[0], m_xMin + m_width);
            ClearColumn(m_pCellGrids[1], m_xMin + m_width);
            break;
        case AdjacencyIndex::BOTTOM_LEFT:
            m_pCellGrids[0][GetOffset(m_xMin - 1, m_yMin + m_height)] = 0;
            m_pCellGrids[1][GetOffset(m_xMin - 1, m_yMin + m_height)] = 0;
            break;
        case AdjacencyIndex::BOTTOM:
            ClearRow(m_pCellGrids[0], m_yMin + m_height);
            ClearRow(m_pCellGrids[1], m_yMin + m_height);
            break;
        case AdjacencyIndex::BOTTOM_RIGHT:
            m_pCellGrids[0][GetOffset(m_xMin + m_width, m_yMin + m_height)] = 0;
            m_pCellGrids[1][GetOffset(m_xMin + m_width, m_yMin + m_height)] = 0;
            break;
        default:
            assert(false);
            break;
        }
    }
}
