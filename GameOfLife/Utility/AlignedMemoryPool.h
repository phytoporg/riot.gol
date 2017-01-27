#pragma once

#include "AlignedBuffer.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace Utility
{
    template <size_t N>
    class AlignedMemoryPool
    {
    public:
        AlignedMemoryPool() = default;

        AlignedMemoryPool(size_t sublockSize, size_t superblockLength)
            : m_sublockSize(sublockSize),
              m_superblockLength(superblockLength)
        {
            if (sublockSize % N)
            {
                throw "Sublock size must be a multiple of alignment.";
            }
        }

        ~AlignedMemoryPool()
        {
            for (auto& superblock : m_superblocks)
            {
                assert(!superblock.Refcount);
                delete[] superblock.pBase;
            }
        }

        AlignedBuffer<N> Allocate()
        {
            if (m_freeBuffers.empty())
            {
                AllocateSuperblock();
            }

            auto alignedBuffer = m_freeBuffers.back();
            m_freeBuffers.pop_back();
            m_usedBuffers.push_back(alignedBuffer);

            return alignedBuffer;
        }

        void Free(const AlignedBuffer<N>& buffer)
        {
            auto usedIt = 
                std::find_if(
                    m_usedBuffers.begin(),
                    m_usedBuffers.end(),
                    [&buffer](const AlignedBuffer<N>& used) 
                    {
                        return buffer.Get() == used.Get();
                    });

            m_usedBuffers.erase(usedIt);
            m_freeBuffers.push_back(buffer);

            // 
            // TODO: Update superblock refcount
            //
            auto superIt = 
                std::find_if(
                    m_superblocks.begin(),
                    m_superblocks.end(),
                    [&buffer,this](const SuperBlock& superblock)
                    {
                        return buffer.Get() >= superblock.pAligned &&
                               buffer.Get() < superblock.pAligned + this->m_superblockLength;
                    });
            assert(superIt != m_superblocks.end());
            assert(superIt->Refcount > 0);
            superIt->Refcount--;

            if (!superIt->Refcount && m_superblocks.size() > 1)
            {
                m_superblocks.erase(superIt);
            }
        }

    private:
        uint8_t* AllocateSuperblock()
        {
            const size_t Length = m_sublockSize * m_superblockLength;

            //
            // What to do in OOM?
            //
            uint8_t* pSuperblock = new uint8_t[Length + (N - 1)];
            memset(pSuperblock, 0, Length + (N - 1));

            size_t base = reinterpret_cast<size_t>(pSuperblock);
            if (base & (N - 1))
            {
                base &= ~(N - 1);
                base += N;
            }

            uint8_t* pAligned = reinterpret_cast<uint8_t*>(base);
            assert(!(base & (N - 1)));
            assert(pAligned >= pSuperblock);
            assert(pSuperblock + N >= pAligned);

            uint8_t* pSublock = pAligned;
            for (size_t i = 0; i < m_superblockLength; ++i)
            {
                m_freeBuffers.emplace_back(pSublock, *this);
                pSublock += m_sublockSize;
            }

            m_superblocks.push_back({ pSuperblock, pAligned, 0 });

            return pAligned;
        }

        //
        // Contains a chunk of sub-blocks, allocated on the N-boundary.
        //
        struct SuperBlock
        {
            //
            // Pointer to the actual base of this block.
            //
            uint8_t* pBase;

            //
            // Pointer to the aligned base.
            //
            uint8_t* pAligned;

            //
            // For auto-cleanup on aligned buffer removal.
            //
            size_t Refcount;
        };

        std::vector<SuperBlock> m_superblocks;

        std::vector<AlignedBuffer<N>> m_freeBuffers;
        std::vector<AlignedBuffer<N>> m_usedBuffers;

        size_t m_sublockSize;
        size_t m_superblockLength;;
    };
}
