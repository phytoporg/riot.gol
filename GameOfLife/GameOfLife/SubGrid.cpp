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
    void ValidateDimensions(int64_t width, int64_t height)
    {
        if (width <= 0)
        {
            throw std::out_of_range("SubGrid width is not positive");
        }

        if (height <= 0)
        {
            throw std::out_of_range("SubGrid height is not positive");
        }
    }

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
        uint8_t neighbors = 0;

        for (int64_t dy = -1; dy <= 1; ++dy)
        {
            for (int64_t dx = -1; dx <= 1; ++dx)
            {
                if (!dx && !dy) 
                {
                    //
                    // Don't count self!
                    //
                    continue; 
                }

                if (subGrid.GetCellState(x + dx, y + dy))
                {
                    neighbors++;
                }
            }
        }

        return neighbors;
    }
}

namespace GameOfLife
{
    SubGrid::SubGrid()
        : RectangularGrid(0, 0, 0, 0)
    {}

    SubGrid::SubGrid(
        uint8_t* ppGrids[2],
        SubGridGraph& graph, 
        int64_t xmin, int64_t width,
        int64_t ymin, int64_t height,
        uint32_t generation
        )
        : RectangularGrid(xmin, width, ymin, height),
          m_generation(generation),
          m_pGridGraph(&graph)
    {
        ValidateDimensions(m_width, m_height);
        m_vertexData.reserve(SUBGRID_WIDTH * SUBGRID_HEIGHT);

        //
        // +2 to make room for ghost buffers.
        //
        m_bufferWidth  = m_width + 2;
        m_bufferHeight = m_height + 2;

        m_pCellGrids[0] = ppGrids[0];
        m_pCellGrids[1] = ppGrids[1];
        m_pCurrentCellGrid = m_pCellGrids[0];

        m_coordinates = std::make_pair(m_xMin, m_yMin);
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
        m_vertexData.emplace_back(x, y);
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
        assert(!(m_bufferWidth & 0x4));
        assert(!(m_bufferHeight & 0x4));

        typedef int64_t SkipPtrType;

        //
        // Check top row first
        //
        uint8_t* pTemp = &m_pCurrentCellGrid[GetOffset(m_xMin - 1, m_yMin - 1)];
        for (int64_t i = 0; i < m_bufferWidth / sizeof(SkipPtrType); i++)
        {
            auto pSkipTemp = reinterpret_cast<SkipPtrType*>(pTemp);
            if (*pSkipTemp) return true;
            pTemp += sizeof(SkipPtrType);
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
        if (!m_pGridGraph->GetNeighborArray(*this, ppNeighbors))
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
        // Need to copy border states to new generation grid as well to correctly
        // identify new neighbors to create after this generation has completed.
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
            m_pCurrentCellGrid,
            OtherPointer(m_pCurrentCellGrid, m_pCellGrids[0], m_pCellGrids[1])
        );

        return static_cast<uint32_t>(m_vertexData.size());
    }

    bool SubGrid::IsNextGenerationNeighbor(AdjacencyIndex adjacency) const
    {
        const uint32_t MagicNumber = 0x010101;

        SubGrid** ppNeighbors;
        if (!m_pGridGraph->GetNeighborArray(*this, ppNeighbors))
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
        case GameOfLife::TOP:
        {
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(LeftX, TopY)];
            for (int64_t i = 0; i < m_width; i++)
            {
                const uint32_t Value = *(reinterpret_cast<uint32_t*>(pStart));
                if (Value == MagicNumber)
                {
                    return true;
                }

                pStart++;
            }
            return !!m_pCurrentCellGrid[GetOffset(BufferLeftX,  BufferTopY)] ||
                   !!m_pCurrentCellGrid[GetOffset(BufferRightX, BufferTopY)];
        }
            break;
        case GameOfLife::BOTTOM:
        {
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(LeftX, BottomY)];
            for (int64_t i = 0; i < m_width; i++)
            {
                const uint32_t Value = *(reinterpret_cast<uint32_t*>(pStart)) >> 8;
                if (Value == MagicNumber)
                {
                    return true;
                }

                pStart++;
            }
            return !!m_pCurrentCellGrid[GetOffset(BufferLeftX,  BufferBottomY)] ||
                   !!m_pCurrentCellGrid[GetOffset(BufferRightX, BufferBottomY)];
        }
            break;

        case GameOfLife::LEFT:
        {
            uint8_t consecutiveCells = 0;
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(LeftX, TopY)];
            for (int64_t i = 0; i < m_height; i++)
            {
                if (*pStart)
                {
                    ++consecutiveCells;
                }
                else
                {
                    consecutiveCells = 0;
                }

                if (consecutiveCells == 3)
                {
                    return true;
                }

                pStart += m_bufferWidth;
            }
            return !!m_pCurrentCellGrid[GetOffset(BufferLeftX, BufferTopY)] ||
                   !!m_pCurrentCellGrid[GetOffset(BufferLeftX, BufferBottomY)];
        }
            break;
        case GameOfLife::RIGHT:
        {
            uint8_t consecutiveCells = 0;
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(RightX, TopY)];
            for (int64_t i = 0; i < m_height; i++)
            {
                if (*pStart)
                {
                    ++consecutiveCells;
                }
                else
                {
                    consecutiveCells = 0;
                }

                if (consecutiveCells == 3)
                {
                    return true;
                }

                pStart += m_bufferWidth;
            }
            return !!m_pCurrentCellGrid[GetOffset(BufferRightX, BufferTopY)] ||
                   !!m_pCurrentCellGrid[GetOffset(BufferRightX, BufferBottomY)];
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
        // interface should refer to generation. This is clunky.
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

    void SubGrid::PublishBorder(SubGrid& other, AdjacencyIndex adjacency)
    {
        assert(m_generation == other.m_generation);
        uint8_t* pNeighborGrid = other.m_pCurrentCellGrid;

        switch (adjacency)
        {
        case AdjacencyIndex::TOP_LEFT:
            pNeighborGrid[other.GetOffset(other.XMin() - 1, other.YMin() - 1)] = 
                GetCellState(
                    m_pCurrentCellGrid,
                    XMin() + Width()  - 1,
                    YMin() + Height() - 1
                    );
            break;
        case AdjacencyIndex::TOP:
            CopyRowFrom(
                *this, m_pCurrentCellGrid,
                other, pNeighborGrid,
                YMin() + Height() - 1,
                other.YMin() - 1
                );
            break;
        case AdjacencyIndex::TOP_RIGHT:
            pNeighborGrid[other.GetOffset(other.XMin() + other.Width(), other.YMin() - 1)] = 
                GetCellState(
                    m_pCurrentCellGrid,
                    XMin(),
                    YMin() + Height() - 1
                    );
            break;
        case AdjacencyIndex::LEFT:
            CopyColumnFrom(
                *this, m_pCurrentCellGrid,
                other, pNeighborGrid,
                XMin() + Width() - 1,
                other.XMin() - 1
                );
            break;
        case AdjacencyIndex::RIGHT:
            CopyColumnFrom(
                *this, m_pCurrentCellGrid,
                other, pNeighborGrid,
                XMin(),
                other.XMin() + other.Width()
                );
            break;
        case AdjacencyIndex::BOTTOM_LEFT:
            pNeighborGrid[other.GetOffset(other.XMin() - 1, other.YMin() + other.Height())] = 
                GetCellState(
                    m_pCurrentCellGrid,
                    XMin() + Width() - 1,
                    YMin()
                    );
            break;
        case AdjacencyIndex::BOTTOM:
            CopyRowFrom(
                *this, m_pCurrentCellGrid,
                other, pNeighborGrid,
                YMin(),
                other.YMin() + other.Height()
                );
            break;
        case AdjacencyIndex::BOTTOM_RIGHT:
            pNeighborGrid[other.GetOffset(other.XMin() + other.Width(), other.YMin() + other.Height())] = 
                GetCellState(
                    m_pCurrentCellGrid,
                    XMin() + Width()  - 1,
                    YMin() + Height() - 1
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
            m_pCurrentCellGrid[GetOffset(m_xMin - 1, m_yMin - 1)] = 0;
            pOtherGrid[GetOffset(m_xMin - 1, m_yMin - 1)] = 0;
            break;
        case AdjacencyIndex::TOP:
            ClearRow(m_pCurrentCellGrid, m_yMin - 1);
            ClearRow(pOtherGrid, m_yMin - 1);
            break;
        case AdjacencyIndex::TOP_RIGHT:
            m_pCurrentCellGrid[GetOffset(m_xMin + m_width, m_yMin - 1)] = 0;
            pOtherGrid[GetOffset(m_xMin + m_width, m_yMin - 1)] = 0;
            break;
        case AdjacencyIndex::LEFT:
            ClearColumn(m_pCurrentCellGrid, m_xMin - 1);
            ClearColumn(pOtherGrid, m_xMin - 1);
            break;
        case AdjacencyIndex::RIGHT:
            ClearColumn(m_pCurrentCellGrid, m_xMin + m_width);
            ClearColumn(pOtherGrid, m_xMin + m_width);
            break;
        case AdjacencyIndex::BOTTOM_LEFT:
            m_pCurrentCellGrid[GetOffset(m_xMin - 1, m_yMin + m_height)] = 0;
            pOtherGrid[GetOffset(m_xMin - 1, m_yMin + m_height)] = 0;
            break;
        case AdjacencyIndex::BOTTOM:
            ClearRow(m_pCurrentCellGrid, m_yMin + m_height);
            ClearRow(pOtherGrid, m_yMin + m_height);
            break;
        case AdjacencyIndex::BOTTOM_RIGHT:
            m_pCurrentCellGrid[GetOffset(m_xMin + m_width, m_yMin + m_height)] = 0;
            pOtherGrid[GetOffset(m_xMin + m_width, m_yMin + m_height)] = 0;
            break;
        default:
            assert(false);
            break;
        }
    }
}
