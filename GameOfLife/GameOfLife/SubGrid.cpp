#include "SubGrid.h"
#include "SubgridGraph.h"

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

    void SubGrid::CopyRowFrom(
        const SubGrid& other,
        uint8_t const* pGrid,
        int64_t ySrc,
        int64_t yDst
        )
    {
        assert(m_width == other.m_width);
        const uint8_t* const pSrc = &pGrid[other.GetOffset(other.m_xMin, ySrc)];
              uint8_t* const pDst = &m_pCurrentCellGrid[GetOffset(m_xMin, yDst)];

        memcpy(pDst, pSrc, m_width);
    }

    void SubGrid::ClearRow(uint8_t* pBuffer, int64_t row)
    {
        uint8_t* const pDst = &pBuffer[GetOffset(m_xMin, row)];
        memset(pDst, 0, m_width);
    }

    void SubGrid::CopyColumnFrom(
        const SubGrid& other,
        uint8_t const* pGrid,
        int64_t xSrc,
        int64_t xDst
        )
    {
        assert(m_height == other.m_height);
        const uint8_t* pSrc = &pGrid[other.GetOffset(xSrc, m_yMin)];
              uint8_t* pDst = &m_pCurrentCellGrid[GetOffset(xDst, other.m_yMin)];

        for (int64_t i = 0; i < m_height; i++)
        {
            *pDst = *pSrc;
            pDst += m_bufferWidth;
            pSrc += other.m_bufferWidth;
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

        switch (adjacency)
        {
            //
            // TODO: These
            //
        case GameOfLife::TOP_LEFT:
            break;
        case GameOfLife::TOP_RIGHT:
            break;
        case GameOfLife::BOTTOM_LEFT:
            break;
        case GameOfLife::BOTTOM_RIGHT:
            break;

        case GameOfLife::TOP:
        {
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(m_xMin, m_yMin)];
            for (int64_t i = 0; i < m_width; i++)
            {
                const uint32_t Value = *(reinterpret_cast<uint32_t*>(pStart));
                if (Value == MagicNumber)
                {
                    return true;
                }

                pStart++;
            }
        }
            break;
        case GameOfLife::BOTTOM:
        {
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(m_xMin, m_yMin + m_height - 1)];
            for (int64_t i = 0; i < m_width; i++)
            {
                const uint32_t Value = *(reinterpret_cast<uint32_t*>(pStart)) >> 8;
                if (Value == MagicNumber)
                {
                    return true;
                }

                pStart++;
            }
        }
            break;

        case GameOfLife::LEFT:
        {
            uint8_t previousValue = 0;
            uint8_t consecutiveCells = 0;
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(m_xMin, m_yMin)];
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

                previousValue = *pStart;
                pStart += m_bufferWidth;
            }
        }
            break;
        case GameOfLife::RIGHT:
        {
            uint8_t previousValue = 0;
            uint8_t consecutiveCells = 0;
            uint8_t* pStart = &m_pCurrentCellGrid[GetOffset(m_xMin + m_width - 1, m_yMin)];
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

                previousValue = *pStart;
                pStart += m_bufferWidth;
            }
        }
            break;
        default:
            assert(false);
            break;
        }

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
                other.XMin() + other.Width() - 1,
                m_xMin - 1
                );
            break;
        case AdjacencyIndex::RIGHT:
            CopyColumnFrom(
                other, pNeighborGrid,
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
                other.YMin(),
                m_yMin + m_height
                );
            break;
        case AdjacencyIndex::BOTTOM_RIGHT:
            m_pCurrentCellGrid[GetOffset(m_xMin + m_width, m_yMin + m_height)] = 
                other.GetCellState(
                    pNeighborGrid,
                    other.XMin() + other.Width()  - 1,
                    other.YMin() + other.Height() - 1
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
